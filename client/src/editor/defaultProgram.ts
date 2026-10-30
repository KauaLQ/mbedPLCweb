import type { Program } from "../engine/types";

// Partida/parada com selo, temporizador e contador (mesma ideia do programa default do firmware)
//   I0 = start | I1 = stop | I2 = conta pulsos
export const defaultProgram = (): Program => ({
  lines: [
    {
      id: "r1",
      logic: {
        id: "n1", type: "OP", op: "AND",
        nodes: [
          {
            id: "n2", type: "OP", op: "OR",
            nodes: [
              { id: "n3", type: "CONTACT", pin: "I0", mode: "NO" },
              { id: "n4", type: "CONTACT", pin: "M0", mode: "NO" },
            ],
          },
          { id: "n5", type: "CONTACT", pin: "I1", mode: "NC" },
        ],
      },
      outputs: [{ id: "o1", type: "COIL", pin: "M0" }],
    },
    {
      id: "r2",
      logic: { id: "n6", type: "CONTACT", pin: "M0", mode: "NO" },
      outputs: [{ id: "o2", type: "TON", pin: "T0", preset: 5000 }],
    },
    {
      id: "r3",
      logic: { id: "n7", type: "COMPARE", left: "T0.ACC", op: ">=", right: "2500" },
      outputs: [{ id: "o3", type: "COIL", pin: "Q1" }],
    },
    {
      id: "r4",
      logic: { id: "n8", type: "CONTACT", pin: "T0.DN", mode: "NO" },
      outputs: [{ id: "o4", type: "COIL", pin: "Q0" }],
    },
    {
      id: "r5",
      logic: { id: "n9", type: "R_TRIG", pin: "I2" },
      outputs: [{ id: "o5", type: "CTU", pin: "C0", preset: 3 }],
    },
    {
      id: "r6",
      logic: { id: "n10", type: "CONTACT", pin: "C0.DN", mode: "NO" },
      outputs: [
        { id: "o6", type: "RES_TIMER", pin: "T0" },
        { id: "o7", type: "RES_COUNTER", pin: "C0" },
      ],
    },
  ],
});
