import { pinsForOutput, needsPreset } from "../engine/tags";
import type { ContactNode, LogicNode, Output, OutputType, Program, Rung } from "../engine/types";

export const uid = () => Math.random().toString(36).slice(2, 9);

export const newContact = (pin = "I0"): ContactNode => ({ id: uid(), type: "CONTACT", pin, mode: "NO" });
export const newOutput = (): Output => ({ id: uid(), type: "COIL", pin: "Q0" });
export const newRung = (): Rung => ({ id: uid(), logic: newContact(), outputs: [newOutput()] });

// ---------------- árvore lógica ----------------
// Achata AND dentro de AND / OR dentro de OR e remove grupos de um único filho
export function normalize(n: LogicNode): LogicNode {
  if (n.type !== "OP") return n;
  const nodes = n.nodes.flatMap((c) => {
    const m = normalize(c);
    return m.type === "OP" && m.op === n.op ? m.nodes : [m];
  });
  return nodes.length === 1 ? nodes[0] : { ...n, nodes };
}

export function findNode(n: LogicNode, id: string): LogicNode | null {
  if (n.id === id) return n;
  if (n.type === "OP") for (const c of n.nodes) { const f = findNode(c, id); if (f) return f; }
  return null;
}

export function updateNode(n: LogicNode, id: string, fn: (x: LogicNode) => LogicNode): LogicNode {
  if (n.id === id) return fn(n);
  if (n.type === "OP") return { ...n, nodes: n.nodes.map((c) => updateNode(c, id, fn)) };
  return n;
}

// Insere `fresh` ao lado de `targetId`: em série (AND) ou em paralelo (OR)
export function addNear(n: LogicNode, targetId: string, op: "AND" | "OR", fresh: LogicNode, before = false): LogicNode {
  if (n.id === targetId) return { id: uid(), type: "OP", op, nodes: before ? [fresh, n] : [n, fresh] };
  if (n.type !== "OP") return n;
  const idx = n.nodes.findIndex((c) => c.id === targetId);
  if (idx >= 0 && n.op === op) {
    const nodes = [...n.nodes];
    nodes.splice(before ? idx : idx + 1, 0, fresh);
    return { ...n, nodes };
  }
  return { ...n, nodes: n.nodes.map((c) => addNear(c, targetId, op, fresh, before)) };
}

export function removeNode(n: LogicNode, id: string): LogicNode | null {
  if (n.id === id) return null;
  if (n.type !== "OP") return n;
  const nodes = n.nodes.map((c) => removeNode(c, id)).filter((c): c is LogicNode => c !== null);
  if (nodes.length === 0) return null;
  return nodes.length === 1 ? nodes[0] : { ...n, nodes };
}

// Troca o tipo do elemento mantendo o id (e a seleção)
export function convertNode(n: LogicNode, type: "CONTACT" | "R_TRIG" | "F_TRIG" | "COMPARE"): LogicNode {
  if (n.type === type || n.type === "OP") return n;
  const pin = n.type === "COMPARE" ? "I0" : n.pin;
  if (type === "CONTACT") return { id: n.id, type, pin, mode: "NO" };
  if (type === "COMPARE") return { id: n.id, type, left: "A0", op: ">", right: "2000" };
  return { id: n.id, type, pin };
}

// ---------------- programa ----------------
const mapRung = (p: Program, rungId: string, fn: (r: Rung) => Rung): Program =>
  ({ lines: p.lines.map((r) => (r.id === rungId ? fn(r) : r)) });

export const addRung = (p: Program): Program => ({ lines: [...p.lines, newRung()] });

export const removeRung = (p: Program, rungId: string): Program =>
  ({ lines: p.lines.filter((r) => r.id !== rungId) });

export function moveRung(p: Program, rungId: string, dir: -1 | 1): Program {
  const i = p.lines.findIndex((r) => r.id === rungId);
  const j = i + dir;
  if (i < 0 || j < 0 || j >= p.lines.length) return p;
  const lines = [...p.lines];
  [lines[i], lines[j]] = [lines[j], lines[i]];
  return { lines };
}

export const editNode = (p: Program, rungId: string, nodeId: string, fn: (x: LogicNode) => LogicNode): Program =>
  mapRung(p, rungId, (r) => ({ ...r, logic: updateNode(r.logic, nodeId, fn) }));

export const addNode = (p: Program, rungId: string, nodeId: string, op: "AND" | "OR"): [Program, string] => {
  const fresh = newContact();
  return [mapRung(p, rungId, (r) => ({ ...r, logic: normalize(addNear(r.logic, nodeId, op, fresh)) })), fresh.id];
};

export const deleteNode = (p: Program, rungId: string, nodeId: string): Program =>
  mapRung(p, rungId, (r) => ({ ...r, logic: removeNode(r.logic, nodeId) ?? newContact() }));

export const addOutput = (p: Program, rungId: string): [Program, string] => {
  const o = newOutput();
  return [mapRung(p, rungId, (r) => ({ ...r, outputs: [...r.outputs, o] })), o.id];
};

export const deleteOutput = (p: Program, rungId: string, outId: string): Program =>
  mapRung(p, rungId, (r) => ({ ...r, outputs: r.outputs.filter((o) => o.id !== outId) }));

export function editOutput(p: Program, rungId: string, outId: string, patch: Partial<Output>): Program {
  return mapRung(p, rungId, (r) => ({
    ...r,
    outputs: r.outputs.map((o) => {
      if (o.id !== outId) return o;
      let next = { ...o, ...patch };
      if (patch.type) {   // ao trocar o tipo, ajusta pino e preset para algo válido
        const pins = pinsForOutput(next.type as OutputType);
        if (!pins.includes(next.pin)) next.pin = pins[0];
        if (needsPreset(next.type as OutputType)) next.preset = next.preset ?? (next.type === "CTU" ? 5 : 3000);
        else next = { id: next.id, type: next.type, pin: next.pin };
      }
      return next;
    }),
  }));
}

// ---------------- arrastar e soltar ----------------
// before/after = em série antes/depois do alvo | above/below = em paralelo acima/abaixo
export type DropZone = "before" | "after" | "above" | "below";
export interface DropHint { rungId: string; nodeId: string; zone: DropZone }

export function collectIds(n: LogicNode): string[] {
  return n.type === "OP" ? [n.id, ...n.nodes.flatMap(collectIds)] : [n.id];
}

export function describeNode(n: LogicNode): string {
  switch (n.type) {
    case "CONTACT": return n.mode === "NC" ? `${n.pin} (NC)` : n.pin;
    case "R_TRIG": return `${n.pin} ↑`;
    case "F_TRIG": return `${n.pin} ↓`;
    case "COMPARE": return `${n.left} ${n.op} ${n.right}`;
    case "OP": return n.op === "AND" ? "bloco em série" : "bloco em paralelo";
  }
}

// Move um elemento (ou grupo) para perto de `to.nodeId`, na mesma linha ou em outra.
// Ex.: OR[AND[I1,I0], I2] + soltar I2 "em paralelo" com I1  ->  AND[OR[I1,I2], I0]
export function moveNode(
  p: Program,
  from: { rungId: string; nodeId: string },
  to: DropHint,
): Program {
  const src = p.lines.find((r) => r.id === from.rungId);
  const dst = p.lines.find((r) => r.id === to.rungId);
  if (!src || !dst) return p;
  const moving = findNode(src.logic, from.nodeId);
  if (!moving) return p;
  if (from.rungId === to.rungId && findNode(moving, to.nodeId)) return p;   // alvo dentro do próprio bloco

  const op = to.zone === "before" || to.zone === "after" ? "AND" : "OR";
  const before = to.zone === "before" || to.zone === "above";
  const srcRest = removeNode(src.logic, from.nodeId) ?? newContact();

  return {
    lines: p.lines.map((r) => {
      if (r.id === from.rungId && r.id === to.rungId) {
        return { ...r, logic: normalize(addNear(srcRest, to.nodeId, op, moving, before)) };
      }
      if (r.id === from.rungId) return { ...r, logic: normalize(srcRest) };
      if (r.id === to.rungId) return { ...r, logic: normalize(addNear(r.logic, to.nodeId, op, moving, before)) };
      return r;
    }),
  };
}
