# mbedPLC · Simulador Ladder (web)

Primeira etapa da interface web: editor e simulador ladder rodando 100% no navegador
(React + Vite + TypeScript, sem backend). Ainda não há parser/export de JSON nem comunicação com o ESP32.

## Rodando

```bash
npm create vite@latest client -- --template react-ts
```

Na pergunta **“Install with npm and start now?”**, responda **No**. Depois:

```bash
cd client
npm install -D "typescript@<6.1.0"
npm install
npm run dev        # http://localhost:5173
npm run build      # typecheck + build de produção em dist/
```

## Estrutura

```
mbedplc-web/
├─ index.html
├─ package.json · tsconfig.json · vite.config.ts · .gitignore
├─ public/                      logos (claro/escuro, fundo transparente) e favicon
└─ src/
   ├─ main.tsx                  entrada
   ├─ App.tsx                   estado do programa, seleção, tema e layout
   ├─ styles.css                tema industrial claro/escuro (variáveis CSS)
   ├─ engine/                   MOTOR (sem React): mesma semântica do firmware
   │  ├─ types.ts               modelo do programa (mesmas chaves do JSON do firmware)
   │  ├─ tags.ts                tags válidas e resolveTag() (I0, T0.DN, C1.ACC, A0, 2500)
   │  └─ plc.ts                 PlcEngine: scan, timers, contadores, bordas, image table
   ├─ editor/                   operações puras sobre o programa
   │  ├─ tree.ts                série/paralelo/excluir, linhas, saídas
   │  ├─ layout.ts              medidas do desenho
   │  ├─ defaultProgram.ts      programa de exemplo (partida/parada + timer + contador)
   │  └─ storage.ts             persistência local (localStorage)
   ├─ hooks/usePlc.ts           loop de scan (10 ms), RUN/STOP, entradas
   └─ components/
      ├─ Header.tsx             marca, RUN/STOP, exemplo, limpar, tema
      ├─ IOPanel.tsx            entradas I0–I3 (botão/chave), analógicas A0–A1, saídas Q0–Q1
      ├─ MonitorPanel.tsx       memórias, timers e contadores ao vivo
      ├─ RungView.tsx           uma linha: trilhos, lógica e coluna de saídas
      ├─ LadderNode.tsx         desenho SVG dos elementos e fluxo de energia
      └─ Inspector.tsx          edição do elemento/saída selecionado
```

## Como usar o editor

- Clique em um contato ou em uma saída para selecioná-lo; edite no **Inspetor**.
- **+ Série** insere um contato depois do selecionado; **+ Paralelo** abre um ramo paralelo.
- **+ saída** (cabeçalho da linha) adiciona COIL, SET, RESET, TON, TOF, CTU, RES_TIMER ou RES_COUNTER.
- Entradas: *botão* só fica ativo enquanto pressionado; *chave* mantém a posição.
- Editar o programa zera o estado da simulação (como um upload no firmware). **Parar** também zera.

## Fidelidade com o firmware

O motor (`engine/plc.ts`) segue o mesmo ciclo: snapshot das entradas → lógica sobre as imagens → saídas.
Bordas são por nó e o primeiro scan só memoriza; `OP` avalia sempre todos os filhos; NO = botão pressionado.
O modelo do programa usa as mesmas chaves do JSON do firmware (`lines`, `logic`, `outputs`, `RES_TIMER`…),
então a integração futura é só serializar (removendo o campo `id`, que o firmware ignora).
