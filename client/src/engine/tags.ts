import type { OutputType } from "./types";

export const NUM = { I: 4, Q: 2, M: 4, T: 4, C: 4, A: 2 } as const;

const range = (p: string, n: number) => Array.from({ length: n }, (_, i) => `${p}${i}`);
const fields = (p: string, n: number, f: string[]) =>
  range(p, n).flatMap((t) => f.map((x) => `${t}.${x}`));

// Listas usadas pelos <select> do inspetor: só existem tags válidas
export const TAGS = {
  contact: [
    ...range("I", NUM.I), ...range("Q", NUM.Q), ...range("M", NUM.M),
    ...fields("T", NUM.T, ["EN", "TT", "DN"]), ...fields("C", NUM.C, ["DN"]),
  ],
  value: [
    ...range("A", NUM.A),
    ...fields("T", NUM.T, ["ACC", "PRE"]), ...fields("C", NUM.C, ["ACC", "PRE"]),
  ],
  writable: [...range("Q", NUM.Q), ...range("M", NUM.M)],
  timer: range("T", NUM.T),
  counter: range("C", NUM.C),
};

export function pinsForOutput(type: OutputType): string[] {
  if (type === "TON" || type === "TOF" || type === "RES_TIMER") return TAGS.timer;
  if (type === "CTU" || type === "RES_COUNTER") return TAGS.counter;
  return TAGS.writable;
}

export const needsPreset = (t: OutputType) => t === "TON" || t === "TOF" || t === "CTU";

export type TagKind = "const" | "I" | "Q" | "M" | "T" | "C" | "A";
export interface TagRef { kind: TagKind; index: number; field: string; value: number }

const cache = new Map<string, TagRef | null>();

// Mesma regra do firmware: "I0", "T0.DN", "C1.ACC", "A0" ou número literal
export function resolveTag(name: string): TagRef | null {
  if (cache.has(name)) return cache.get(name)!;
  let ref: TagRef | null = null;

  if (/^-?\d+$/.test(name)) {
    ref = { kind: "const", index: 0, field: "", value: parseInt(name, 10) };
  } else {
    const m = /^([IQMTCA])(\d+)(?:\.(EN|TT|DN|ACC|PRE))?$/.exec(name);
    if (m) {
      const kind = m[1] as Exclude<TagKind, "const">;
      const index = parseInt(m[2], 10);
      const field = m[3] ?? "";
      const okField = field === "" || kind === "T" || (kind === "C" && field !== "EN" && field !== "TT");
      if (index < NUM[kind] && okField) ref = { kind, index, field, value: 0 };
    }
  }
  cache.set(name, ref);
  return ref;
}
