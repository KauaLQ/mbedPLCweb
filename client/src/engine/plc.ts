import { NUM, resolveTag } from "./tags";
import type { LogicNode, Output, Program, Rung } from "./types";

export interface TimerState { EN: boolean; TT: boolean; DN: boolean; preset: number; accum: number; startTime: number }
export interface CounterState { DN: boolean; ACC: number; PRE: number; prevCU: boolean }

const newTimer = (): TimerState => ({ EN: false, TT: false, DN: false, preset: 0, accum: 0, startTime: 0 });
const newCounter = (): CounterState => ({ DN: false, ACC: 0, PRE: 0, prevCU: false });
const fill = <T,>(n: number, f: () => T) => Array.from({ length: n }, f);

export interface Snapshot {
  inputs: boolean[];
  analog: number[];
  outputs: boolean[];
  memory: boolean[];
  timers: TimerState[];
  counters: CounterState[];
  results: Map<string, boolean>;   // resultado de cada nó no último scan (para o fluxo de energia)
  scans: number;
}

/**
 * Motor de scan com a mesma semântica do firmware:
 *  1. snapshot das entradas  2. lógica sobre as imagens  3. escrita das saídas.
 * - estado de borda (R_TRIG/F_TRIG) é por nó e o primeiro scan só memoriza;
 * - OP avalia todos os filhos (sem short-circuit);
 * - uma troca de programa zera todo o estado.
 */
export class PlcEngine {
  program: Program = { lines: [] };

  // entradas físicas (o painel de I/O escreve aqui)
  inputs = fill(NUM.I, () => false);
  analog = fill(NUM.A, () => 0);

  private inputImage = fill(NUM.I, () => false);
  private analogImage = fill(NUM.A, () => 0);
  private outputs = fill(NUM.Q, () => false);
  private memory = fill(NUM.M, () => false);
  private timers = fill(NUM.T, newTimer);
  private counters = fill(NUM.C, newCounter);
  private edges = new Map<string, { prev: boolean; primed: boolean }>();
  private results = new Map<string, boolean>();
  private scans = 0;
  private now = 0;

  load(program: Program) {
    this.program = program;
    this.reset();
  }

  reset() {
    this.outputs = fill(NUM.Q, () => false);
    this.memory = fill(NUM.M, () => false);
    this.timers = fill(NUM.T, newTimer);
    this.counters = fill(NUM.C, newCounter);
    this.edges.clear();
    this.results = new Map();
    this.scans = 0;
  }

  scan(now: number) {
    this.now = Math.floor(now);   // ms inteiros (performance.now() é fracionário)
    this.inputImage = [...this.inputs];       // 1. snapshot
    this.analogImage = [...this.analog];
    const results = new Map<string, boolean>();
    this.results = results;
    for (const rung of this.program.lines) this.executeRung(rung);   // 2. lógica
    this.scans++;                             // 3. saídas já estão em this.outputs
  }

  snapshot(): Snapshot {
    return {
      inputs: [...this.inputs],
      analog: [...this.analog],
      outputs: [...this.outputs],
      memory: [...this.memory],
      // ACC só para exibição: limitado ao preset (como num CLP real); o motor usa o valor bruto
      timers: this.timers.map((t) => ({ ...t, accum: Math.min(t.accum, t.preset) })),
      counters: this.counters.map((c) => ({ ...c })),
      results: new Map(this.results),
      scans: this.scans,
    };
  }

  // ---------------- tags ----------------
  private readBool(name: string): boolean {
    const t = resolveTag(name);
    if (!t) return false;
    switch (t.kind) {
      case "I": return this.inputImage[t.index];
      case "Q": return this.outputs[t.index];
      case "M": return this.memory[t.index];
      case "T": return t.field === "EN" ? this.timers[t.index].EN
                     : t.field === "TT" ? this.timers[t.index].TT
                     : t.field === "DN" ? this.timers[t.index].DN : false;
      case "C": return t.field === "DN" && this.counters[t.index].DN;
      default: return false;
    }
  }

  private readValue(name: string): number {
    const t = resolveTag(name);
    if (!t) return 0;
    switch (t.kind) {
      case "const": return t.value;
      case "A": return this.analogImage[t.index];
      case "T": return t.field === "ACC" ? this.timers[t.index].accum : t.field === "PRE" ? this.timers[t.index].preset : 0;
      case "C": return t.field === "ACC" ? this.counters[t.index].ACC : t.field === "PRE" ? this.counters[t.index].PRE : 0;
      default: return 0;
    }
  }

  private write(name: string, value: boolean) {
    const t = resolveTag(name);
    if (!t) return;
    if (t.kind === "Q") this.outputs[t.index] = value;
    else if (t.kind === "M") this.memory[t.index] = value;
  }

  // ---------------- lógica ----------------
  private edge(id: string, pin: string, rising: boolean): boolean {
    const cur = this.readBool(pin);
    let st = this.edges.get(id);
    if (!st) { st = { prev: false, primed: false }; this.edges.set(id, st); }
    const hit = st.primed && (rising ? !st.prev && cur : st.prev && !cur);
    st.prev = cur;
    st.primed = true;
    return hit;
  }

  private evaluate(n: LogicNode): boolean {
    let v = false;
    switch (n.type) {
      case "CONTACT": { const b = this.readBool(n.pin); v = n.mode === "NC" ? !b : b; break; }
      case "R_TRIG": v = this.edge(n.id, n.pin, true); break;
      case "F_TRIG": v = this.edge(n.id, n.pin, false); break;
      case "COMPARE": {
        const l = this.readValue(n.left), r = this.readValue(n.right);
        v = n.op === ">" ? l > r : n.op === "<" ? l < r : n.op === ">=" ? l >= r
          : n.op === "<=" ? l <= r : n.op === "==" ? l === r : l !== r;
        break;
      }
      case "OP": {
        v = n.op === "AND";
        for (const c of n.nodes) {               // sempre avalia todos os filhos
          const cv = this.evaluate(c);
          v = n.op === "AND" ? v && cv : v || cv;
        }
        break;
      }
    }
    this.results.set(n.id, v);
    return v;
  }

  private executeRung(rung: Rung) {
    const result = this.evaluate(rung.logic);
    for (const o of rung.outputs) this.executeOutput(o, result);
  }

  private executeOutput(o: Output, result: boolean) {
    const idx = parseInt(o.pin.slice(1), 10);
    switch (o.type) {
      case "COIL": this.write(o.pin, result); break;
      case "SET": if (result) this.write(o.pin, true); break;
      case "RESET": if (result) this.write(o.pin, false); break;
      case "TON": this.ton(idx, result, o.preset ?? 0); break;
      case "TOF": this.tof(idx, result, o.preset ?? 0); break;
      case "CTU": this.ctu(idx, result, o.preset ?? 0); break;
      case "RES_TIMER": if (result) this.timers[idx] = newTimer(); break;
      case "RES_COUNTER": if (result) { this.counters[idx].ACC = 0; this.counters[idx].DN = false; } break;
    }
  }

  private ton(i: number, cond: boolean, preset: number) {
    const t = this.timers[i];
    t.preset = preset;
    if (cond) {
      if (!t.EN) { t.EN = true; t.startTime = this.now; }
      t.accum = this.now - t.startTime;
      if (t.accum >= t.preset) { t.DN = true; t.TT = false; }
      else { t.TT = true; t.DN = false; }
    } else {
      t.EN = false; t.TT = false; t.DN = false; t.accum = 0;
    }
  }

  private tof(i: number, cond: boolean, preset: number) {
    const t = this.timers[i];
    t.preset = preset;
    if (cond) {
      t.EN = true; t.DN = true; t.TT = false; t.accum = 0;
    } else if (t.EN) {
      if (!t.TT) { t.startTime = this.now; t.TT = true; }
      t.accum = this.now - t.startTime;
      if (t.accum >= t.preset) { t.DN = false; t.TT = false; t.EN = false; }
    }
  }

  private ctu(i: number, cond: boolean, preset: number) {
    const c = this.counters[i];
    c.PRE = preset;
    if (cond && !c.prevCU) {
      c.ACC++;
      if (c.ACC >= c.PRE) c.DN = true;
    }
    c.prevCU = cond;
  }
}
