# Checkers MCTS 🧠♟️

Progetto universitario per lo sviluppo di un Agente Intelligente (Artificial Player) capace di giocare alla **Dama Italiana** a livello esperto. 
Il cuore matematico del bot è interamente sviluppato in **C puro**, utilizzando una rappresentazione iper-ottimizzata a **Bitboard** (maschere di bit a 32-bit) e implementando l'algoritmo **Monte Carlo Tree Search (MCTS)** con un **Memory Pool** pre-allocato per azzerare i ritardi di elaborazione.

L'interfaccia utente è sviluppata in **React** + **Tailwind CSS**. 
Per rispettare i vincoli progettuali di implementare la logica nativamente in C, il motore di calcolo è stato compilato in **WebAssembly (WASM)** tramite *Emscripten*, permettendo all'applicazione Web di far girare l'engine nativo in C direttamente nel browser a prestazioni massime.

> 📑 **Presentazione del Progetto:** Puoi consultare le slide complete della presentazione accademica in [**docs/presentazione/presentazione_iw.pdf**](docs/presentazione/presentazione_iw.pdf).

---

## 🛠️ Requisiti di Sistema
- **Git**
- **Docker** (o Docker Desktop su macOS/Windows) - *Richiesto per la compilazione WebAssembly dell'engine C.*
- **Node.js** (v18+) e **npm** - *Richiesti per l'interfaccia React.*
- **GCC** e **Make** - *Richiesti solo se si vuole testare l'engine C da linea di comando senza interfaccia grafica.*

---

## 🚀 Installazione ed Esecuzione Rapida

### 1. Clona il Repository
Apri il terminale e clona il progetto da GitHub:
```bash
git clone https://github.com/SimoneIng/checkers-MCTS.git
cd checkers-MCTS
```

### 2. Compila il Motore C in WebAssembly
Assicurati di avere **Docker in esecuzione**. Lanceremo un container che in modo del tutto automatico trasformerà il nostro codice C nativo in un velocissimo file `.wasm` inserendolo direttamente nella cartella React.
```bash
docker compose run --rm wasm-builder
```
*(Al termine del comando, verranno creati i file `engine.wasm` e `engine.js` all'interno della cartella `web/public/wasm/`)*

### 3. Avvia l'Interfaccia Grafica
Spostati nella cartella Web, installa le dipendenze di React e lancia il server di sviluppo:
```bash
cd web
npm install
npm run dev
```

Apri il browser all'indirizzo **`http://localhost:5173`**. 
L'interfaccia grafica si connetterà all'Engine C-Wasm e potrai iniziare a giocare a Dama contro il bot in tempo reale, potendo anche configurare a piacimento gli algoritmi (**UCB1** o **PUCT**) e il limite di tempo (**0.2s, 1.0s, 3.0s**).

---

## 💻 Modalità "Sviluppatore" (Terminale puro)
Se desideri eseguire il motore MCTS nudo e crudo senza la GUI Web per analizzare le perfomance del C puro (Self-Play del bot):

```bash
make clean && make
./build/checkers_app
```
Questo comando invocherà il `Makefile` e farà scontrare due agenti (Bianco contro Nero) stampando l'animazione dell'albero delle mosse sul terminale.

---

## 🏗 Struttura del Progetto
- `src/` e `include/`: Logica pura del gioco in C (Bitboard, Generazione mosse, Game State).
- `src/mcts/`: Algoritmo Monte Carlo Tree Search, Memory Pool e iteratore Anytime.
- `src/wasm_api.c`: Interfaccia C (API) per l'interscambio di dati verso Javascript.
- `web/`: Interfaccia utente in React + Tailwind.
- `training/`: Pipeline Python di Best Arm Identification, testing statistico e generazione grafici.
- `training_results/`: Risultati sperimentali delle sessioni di tuning (JSON e figure ad alta risoluzione).
- `docs/`: Documentazione teorica, report analitici (`docs/analysis/`) e slide della [presentazione (PDF)](docs/presentazione/presentazione_iw.pdf).
- `docker-compose.yml`: Script Docker per astrarre la catena di compilazione Emscripten.
