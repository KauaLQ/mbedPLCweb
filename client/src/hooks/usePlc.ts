import { useCallback, useEffect, useRef, useState } from "react";
import { PlcEngine, type Snapshot } from "../engine/plc";
import type { Program } from "../engine/types";

export const SCAN_MS = 10;       // mesmo período do firmware
const UI_EVERY = 3;              // atualiza a tela a cada 3 scans (~30 ms)

export function usePlc(program: Program) {
  const engine = useRef<PlcEngine>(new PlcEngine()).current;
  const [running, setRunning] = useState(true);
  const [snap, setSnap] = useState<Snapshot>(() => engine.snapshot());

  // Programa novo (ou editado): o estado é zerado, como no firmware
  useEffect(() => {
    engine.load(program);
    setSnap(engine.snapshot());
  }, [engine, program]);

  useEffect(() => {
    if (!running) return;
    let n = 0;
    const id = setInterval(() => {
      engine.scan(performance.now());
      if (++n % UI_EVERY === 0) setSnap(engine.snapshot());
    }, SCAN_MS);
    return () => clearInterval(id);
  }, [engine, running]);

  const setInput = useCallback((i: number, v: boolean) => {
    engine.inputs[i] = v;
    setSnap(engine.snapshot());
  }, [engine]);

  const setAnalog = useCallback((i: number, v: number) => {
    engine.analog[i] = v;
    setSnap(engine.snapshot());
  }, [engine]);

  const run = useCallback(() => setRunning(true), []);
  const stop = useCallback(() => {
    setRunning(false);
    engine.reset();
    setSnap(engine.snapshot());
  }, [engine]);

  return { snap, running, run, stop, setInput, setAnalog };
}
