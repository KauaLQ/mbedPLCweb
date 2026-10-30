import type { LogicNode } from "../engine/types";

// Medidas do desenho (px). Todo elemento tem o fio na altura WIRE_Y.
export const ELEM_W = 96;
export const ELEM_H = 72;
export const WIRE_Y = 40;
export const PAD = 14;        // recuo das barras verticais de um paralelo
export const OUT_W = 132;
export const RAIL_X = 14;     // posição da barra de energia esquerda

export interface Size { w: number; h: number }

export function measure(n: LogicNode): Size {
  if (n.type !== "OP") return { w: ELEM_W, h: ELEM_H };
  const ms = n.nodes.map(measure);
  if (n.op === "AND") {
    return { w: ms.reduce((a, m) => a + m.w, 0), h: Math.max(...ms.map((m) => m.h)) };
  }
  return { w: Math.max(...ms.map((m) => m.w)) + 2 * PAD, h: ms.reduce((a, m) => a + m.h, 0) };
}
