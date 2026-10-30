// Modelo do programa: mesmas chaves do JSON do firmware (lines / logic / outputs).
// O campo `id` existe só na interface; o firmware simplesmente o ignora.

export type Mode = "NO" | "NC";
export type CmpOp = ">" | "<" | ">=" | "<=" | "==" | "!=";

export interface ContactNode { id: string; type: "CONTACT"; pin: string; mode: Mode }
export interface TrigNode    { id: string; type: "R_TRIG" | "F_TRIG"; pin: string }
export interface CompareNode { id: string; type: "COMPARE"; left: string; op: CmpOp; right: string }
export interface OpNode      { id: string; type: "OP"; op: "AND" | "OR"; nodes: LogicNode[] }
export type LogicNode = ContactNode | TrigNode | CompareNode | OpNode;

export type OutputType =
  | "COIL" | "SET" | "RESET"
  | "TON" | "TOF" | "CTU"
  | "RES_TIMER" | "RES_COUNTER";

export interface Output { id: string; type: OutputType; pin: string; preset?: number }
export interface Rung   { id: string; logic: LogicNode; outputs: Output[] }
export interface Program { lines: Rung[] }
