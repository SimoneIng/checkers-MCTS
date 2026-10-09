import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import './index.css';

function App() {
  const [timeLimit, setTimeLimit] = useState(0.5);
  const [policy, setPolicy] = useState(0); 
  const [explorationC, setExplorationC] = useState(1.414);
  
  // Game State
  const [boardState, setBoardState] = useState({ white: 0, black: 0, kings: 0, turn: 1, status: 0 });
  const [selectedSquare, setSelectedSquare] = useState(null);
  const [legalMovesStr, setLegalMovesStr] = useState("");
  const [isEngineReady, setIsEngineReady] = useState(false);
  
  // UX State
  const [isMenuOpen, setIsMenuOpen] = useState(true);
  const [lastMove, setLastMove] = useState({ from: null, to: null, captures: 0 });
  const [forcedCaptureSquares, setForcedCaptureSquares] = useState([]);

  // Effetti Sonori
  const playSound = (isCapture) => {
    const ctx = new (window.AudioContext || window.webkitAudioContext)();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    
    if (isCapture) {
      osc.type = 'square';
      osc.frequency.setValueAtTime(150, ctx.currentTime);
      osc.frequency.exponentialRampToValueAtTime(40, ctx.currentTime + 0.1);
      gain.gain.setValueAtTime(0.3, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.1);
      osc.start(ctx.currentTime);
      osc.stop(ctx.currentTime + 0.1);
    } else {
      osc.type = 'sine';
      osc.frequency.setValueAtTime(400, ctx.currentTime);
      gain.gain.setValueAtTime(0.1, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.05);
      osc.start(ctx.currentTime);
      osc.stop(ctx.currentTime + 0.05);
    }
  };

  // Inizializza WebAssembly
  useEffect(() => {
    const checkModule = setInterval(() => {
      if (window.Module && window.Module._init_engine) {
        clearInterval(checkModule);
        window.Module._init_engine();
        // Set the board visually but don't "start" the bot yet
        window.Module._start_game();
        syncBoardFromEngine();
        setIsEngineReady(true);
      }
    }, 100);
    return () => clearInterval(checkModule);
  }, []);

  const syncBoardFromEngine = () => {
    setBoardState({
      white: window.Module._get_white_pieces(),
      black: window.Module._get_black_pieces(),
      kings: window.Module._get_kings(),
      turn: window.Module._get_current_player(),
      status: window.Module._get_game_status()
    });
    
    const ptr = window.Module._get_legal_moves_string();
    const str = window.Module.UTF8ToString(ptr);
    setLegalMovesStr(str);
    
    const moves = str.split(',').filter(m => m.trim() !== '');
    const forced = [];
    moves.forEach(m => {
      const [from, to, caps] = m.split('-').map(Number);
      if (caps > 0) {
        if (!forced.includes(from)) forced.push(from);
      }
    });
    setForcedCaptureSquares(forced);
  };

  const startNewGame = () => {
    window.Module._start_game();
    setLastMove({ from: null, to: null, captures: 0 });
    syncBoardFromEngine();
    setSelectedSquare(null);
    setIsMenuOpen(false); // Chiudi il menu
  };

  // Fa giocare il bot solo se il menu è chiuso
  useEffect(() => {
    if (boardState.turn === 0 && boardState.status === 0 && isEngineReady && !isMenuOpen) {
      setTimeout(() => {
        const res = window.Module._play_bot_move(timeLimit, policy, explorationC);
        
        const from = (res >> 8) & 0xFF;
        const to = res & 0xFF;
        const caps = res >> 16;
        
        setLastMove({ from, to, captures: caps });
        playSound(caps > 0);
        
        syncBoardFromEngine();
      }, 100); 
    }
  }, [boardState.turn, boardState.status, isEngineReady, isMenuOpen, timeLimit, policy, explorationC]);

  const handleSquareClick = (idx) => {
    if (boardState.turn !== 1 || boardState.status !== 0 || isMenuOpen) return;
    
    const isWhite = (boardState.white & (1 << idx)) !== 0;
    
    if (isWhite) {
      if (forcedCaptureSquares.length > 0 && !forcedCaptureSquares.includes(idx)) {
        return;
      }
      setSelectedSquare(idx);
    } else if (selectedSquare !== null) {
      const res = window.Module._play_human_move(selectedSquare, idx);
      if (res !== -1) {
        const caps = res >> 16;
        setLastMove({ from: selectedSquare, to: idx, captures: caps });
        playSound(caps > 0);
        syncBoardFromEngine();
        setSelectedSquare(null); 
      } else {
        // Se l'utente ha cliccato una casella non valida, verifichiamo se voleva cliccare un pallino intermedio
        const isIntermediate = getTargetDots().has(idx);
        if (isIntermediate) {
          // Ignora il click per non far perdere la selezione (l'utente capirà di dover cliccare quello finale)
          return;
        }
        setSelectedSquare(null);
      }
    }
  };

  // ---- LOGICA UI PER SALTI MULTIPLI ----
  // Trova le caselle intermedie geometriche di un salto multiplo (DFS semplice)
  const findIntermediateSquares = (start, end, caps, isWhite) => {
    if (caps <= 1) return [];
    
    const getRowCol = (idx) => {
      const r = Math.floor(idx / 4);
      const c = (idx % 4) * 2 + (r % 2 === 0 ? 0 : 1);
      return { r, c };
    };
    
    const getIdx = (r, c) => {
      if (r < 0 || r > 7 || c < 0 || c > 7) return -1;
      if ((r + c) % 2 !== 0) return -1;
      return r * 4 + Math.floor(c / 2);
    };

    const isOpponent = (idx) => {
      const mask = 1 << idx;
      if (isWhite) return (boardState.black & mask) !== 0;
      return (boardState.white & mask) !== 0;
    };

    const isEmpty = (idx) => {
      const mask = 1 << idx;
      return (boardState.white & mask) === 0 && (boardState.black & mask) === 0;
    };

    const isKing = (boardState.kings & (1 << start)) !== 0;
    const dirs = isKing ? [[1, -1], [1, 1], [-1, -1], [-1, 1]] : (isWhite ? [[1, -1], [1, 1]] : [[-1, -1], [-1, 1]]);

    let foundPath = [];
    
    const dfs = (curr, currentCaps, path) => {
      if (currentCaps === caps && curr === end) {
        foundPath = [...path];
        return true;
      }
      if (currentCaps >= caps) return false;

      const { r, c } = getRowCol(curr);
      for (const [dr, dc] of dirs) {
        const midIdx = getIdx(r + dr, c + dc);
        const landIdx = getIdx(r + 2 * dr, c + 2 * dc);
        
        if (midIdx !== -1 && landIdx !== -1) {
          if (isOpponent(midIdx) && (isEmpty(landIdx) || landIdx === end)) {
            // Per evitare loop infiniti nei re, potremmo tenere traccia dei mangiati
            // ma per una profondità massima di 3-4 salti, va bene così per la UI.
            path.push(landIdx);
            if (dfs(landIdx, currentCaps + 1, path)) return true;
            path.pop();
          }
        }
      }
      return false;
    };

    dfs(start, 0, []);
    return foundPath.slice(0, -1); // Rimuove l'endIdx finale dalla lista degli intermedi
  };

  const getTargetDots = () => {
    let dots = new Set();
    if (selectedSquare === null) return dots;

    const moves = legalMovesStr.split(',').filter(m => m.trim() !== '');
    moves.forEach(m => {
      const [from, to, caps] = m.split('-').map(Number);
      if (from === selectedSquare) {
        dots.add(to);
        if (caps > 1) {
          const isWhite = (boardState.white & (1 << from)) !== 0;
          const intermediates = findIntermediateSquares(from, to, caps, isWhite);
          intermediates.forEach(idx => dots.add(idx));
        }
      }
    });
    return dots;
  };

  const activeDots = getTargetDots();

  return (
    <div className="flex flex-col items-center justify-center h-screen w-full bg-gray-100 font-sans relative overflow-hidden">
      
      {/* Intestazione e Bottone Menu */}
      <div className="absolute top-6 flex justify-between items-center w-full max-w-2xl px-4 z-10">
        <h1 className="text-4xl font-black text-gray-800 tracking-widest drop-shadow-sm opacity-90">MCTS CHECKERS</h1>
        <button 
          onClick={() => setIsMenuOpen(true)}
          className="bg-white/50 hover:bg-white text-gray-700 p-3 rounded-full backdrop-blur-md transition-all shadow-sm border border-gray-300"
        >
          <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z"></path><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"></path></svg>
        </button>
      </div>

      {/* OVERLAY MENU (Modal) */}
      <AnimatePresence>
        {isMenuOpen && (
          <motion.div 
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="absolute inset-0 z-50 flex items-center justify-center bg-gray-500/30 backdrop-blur-sm"
          >
            <motion.div 
              initial={{ scale: 0.9, y: 20 }}
              animate={{ scale: 1, y: 0 }}
              exit={{ scale: 0.9, y: 20 }}
              className="bg-white rounded-2xl shadow-2xl p-8 w-full max-w-md mx-4 border border-white/50"
            >
              <div className="text-center mb-8">
                <h2 className="text-3xl font-bold text-gray-800">Impostazioni</h2>
                <p className="text-sm text-green-600 font-semibold mt-1">
                  {isEngineReady ? "Motore C WebAssembly Connesso 🟢" : "Inizializzazione Motore... ⏳"}
                </p>
              </div>

              <div className="flex flex-col gap-6">
                <div className="flex flex-col gap-2">
                  <label className="text-sm font-semibold text-gray-700">Algoritmo di Selezione</label>
                  <div className="flex bg-gray-100 rounded-lg p-1">
                    <button onClick={() => setPolicy(0)} className={`flex-1 py-2 rounded-md font-medium text-sm transition-all ${policy === 0 ? 'bg-white shadow-sm text-blue-600' : 'text-gray-500 hover:text-gray-700'}`}>UCB1 (Classico)</button>
                    <button onClick={() => setPolicy(1)} className={`flex-1 py-2 rounded-md font-medium text-sm transition-all ${policy === 1 ? 'bg-white shadow-sm text-blue-600' : 'text-gray-500 hover:text-gray-700'}`}>PUCT (AlphaGo)</button>
                  </div>
                </div>

                <div className="flex flex-col gap-2">
                  <label className="text-sm font-semibold text-gray-700">Tempo del Bot per mossa</label>
                  <div className="grid grid-cols-3 gap-2">
                    {[0.2, 1.0, 3.0].map((t) => (
                      <button key={t} onClick={() => setTimeLimit(t)} className={`py-2 rounded-lg font-bold transition-all border ${timeLimit === t ? 'bg-blue-50 border-blue-200 text-blue-700 shadow-sm ring-1 ring-blue-500' : 'bg-white border-gray-200 text-gray-600 hover:bg-gray-50'}`}>{t}s</button>
                    ))}
                  </div>
                </div>

                <div className="flex flex-col gap-2">
                  <div className="flex justify-between items-center">
                    <label className="text-sm font-semibold text-gray-700">Costante di Esplorazione</label>
                    <span className="text-sm font-mono bg-blue-100 text-blue-800 px-2 py-1 rounded-md">{explorationC.toFixed(2)}</span>
                  </div>
                  <input type="range" min="0.1" max="3.0" step="0.1" value={explorationC} onChange={(e) => setExplorationC(Number(e.target.value))} className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer accent-blue-600" />
                </div>
              </div>

              {/* Bottoni Azione */}
              <div className="mt-8 flex flex-col gap-3">
                <button 
                  onClick={startNewGame} 
                  disabled={!isEngineReady}
                  className="w-full bg-gradient-to-r from-blue-600 to-indigo-600 hover:from-blue-700 hover:to-indigo-700 disabled:from-gray-400 disabled:to-gray-400 text-white font-bold py-4 rounded-xl shadow-lg transition-transform transform hover:scale-[1.02] active:scale-95 flex items-center justify-center gap-2"
                >
                  <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 20 20"><path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clipRule="evenodd"></path></svg>
                  {isEngineReady ? "Inizia Nuova Partita" : "Attendere..."}
                </button>

                {isEngineReady && boardState.status === 0 && (
                  <div className="flex gap-3">
                    <button 
                      onClick={() => setIsMenuOpen(false)} 
                      className="flex-1 bg-white border-2 border-gray-200 hover:border-gray-300 text-gray-700 font-bold py-3 rounded-xl shadow-sm transition-transform transform hover:scale-[1.02] active:scale-95"
                    >
                      Riprendi
                    </button>
                    <button 
                      onClick={() => {
                        setBoardState(prev => ({ ...prev, status: 2 })); // Forza la vittoria del bot
                        setIsMenuOpen(false);
                      }} 
                      className="flex-1 bg-white border-2 border-red-100 hover:bg-red-50 text-red-600 font-bold py-3 rounded-xl shadow-sm transition-transform transform hover:scale-[1.02] active:scale-95"
                    >
                      Abbandona
                    </button>
                  </div>
                )}
              </div>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>

      {/* Banner Risultato (se partita finita) */}
      <AnimatePresence>
        {boardState.status > 0 && !isMenuOpen && (
          <motion.div 
            initial={{ opacity: 0, y: -50 }}
            animate={{ opacity: 1, y: 0 }}
            className="absolute top-24 z-20 w-full max-w-md"
          >
            <div className={`p-4 rounded-2xl shadow-2xl text-center border-2 backdrop-blur-md font-bold text-2xl
              ${boardState.status === 1 ? 'bg-green-500/80 border-green-400 text-white' : 
                boardState.status === 2 ? 'bg-red-500/80 border-red-400 text-white' : 
                'bg-yellow-500/80 border-yellow-400 text-white'}`}
            >
              {boardState.status === 1 ? "🎉 VITTORIA EROICA!" : boardState.status === 2 ? "💀 SEI STATO DISTRUTTO" : "🤝 PATTA INEVITABILE"}
              <button onClick={() => setIsMenuOpen(true)} className="block w-full mt-4 text-sm bg-white/20 hover:bg-white/30 py-2 rounded-xl transition-colors">Visualizza Menu</button>
            </div>
          </motion.div>
        )}
      </AnimatePresence>

      {/* AREA DI GIOCO */}
      <div className={`w-full max-w-2xl aspect-square shadow-2xl border-4 border-[#3e2723] rounded-sm grid grid-cols-8 grid-rows-8 transition-all duration-500 ${isMenuOpen ? 'blur-sm scale-95 opacity-50' : 'blur-0 scale-100'}`}>
        
        {Array.from({ length: 64 }).map((_, i) => {
          const row = Math.floor(i / 8);
          const col = i % 8;
          const logicRow = 7 - row;
          const isDark = (logicRow + col) % 2 === 0;
          
          let squareIdx = null;
          let content = null;
          let isForced = false;

          if (isDark) {
            squareIdx = logicRow * 4 + Math.floor(col / 2);
            const mask = 1 << squareIdx;
            const hasWhite = (boardState.white & mask) !== 0;
            const hasBlack = (boardState.black & mask) !== 0;
            const isKing = (boardState.kings & mask) !== 0;
            
            isForced = forcedCaptureSquares.includes(squareIdx);
            const isDoubleCaptureAnim = lastMove.to === squareIdx && lastMove.captures > 1;

            if (hasWhite) {
              content = (
                <div className={`w-[80%] h-[80%] rounded-full bg-white shadow-[0_4px_6px_rgba(0,0,0,0.5),_inset_0_-4px_0_rgba(200,200,200,0.6)] border border-gray-300 flex items-center justify-center transition-transform duration-300 z-10 ${isDoubleCaptureAnim ? 'animate-bounce' : ''}`}>
                  {isKing && <div className="w-1/2 h-1/2 rounded-full border-[5px] border-gray-300"></div>}
                </div>
              );
            } else if (hasBlack) {
              content = (
                <div className={`w-[80%] h-[80%] rounded-full bg-gray-900 shadow-[0_4px_6px_rgba(0,0,0,0.7),_inset_0_-4px_0_rgba(60,60,60,0.8)] border border-black flex items-center justify-center transition-transform duration-300 z-10 ${isDoubleCaptureAnim ? 'animate-bounce' : ''}`}>
                  {isKing && <div className="w-1/2 h-1/2 rounded-full border-[5px] border-gray-600"></div>}
                </div>
              );
            }
          }

          const isSelected = selectedSquare === squareIdx;
          const hasDot = activeDots.has(squareIdx);

          // Pallino in stile chess.com
          let dot = null;
          if (hasDot) {
            dot = <div className="absolute w-[30%] h-[30%] rounded-full bg-white/70 z-20 pointer-events-none"></div>;
          }

          return (
            <div 
              key={i} 
              onClick={() => isDark && handleSquareClick(squareIdx)}
              className={`w-full h-full flex items-center justify-center relative
                ${isDark ? 'bg-[#8B4513] cursor-pointer' : 'bg-white'}
                ${isForced && !isSelected ? 'ring-inset ring-4 ring-red-500 animate-pulse' : ''}
              `}
            >
              {/* Highlight giallo quando selezionata (sovrapposto al colore base) */}
              {isSelected && isDark && <div className="absolute inset-0 bg-yellow-400/50 z-0 pointer-events-none"></div>}
              
              {content}
              {dot}
            </div>
          );
        })}
      </div>
      
    </div>
  );
}

export default App;
