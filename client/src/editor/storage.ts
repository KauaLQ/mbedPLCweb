import type { Program } from "../engine/types";

const KEY = "mbedplc.program.v1";

export function loadProgram(): Program | null {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) return null;
    const p = JSON.parse(raw);
    return Array.isArray(p?.lines) ? (p as Program) : null;
  } catch {
    return null;
  }
}

export function saveProgram(p: Program) {
  try {
    localStorage.setItem(KEY, JSON.stringify(p));
  } catch {
    /* sem armazenamento disponível: segue sem persistir */
  }
}
