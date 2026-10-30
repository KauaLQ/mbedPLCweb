import { useState } from "react";
import type { Snapshot } from "../engine/plc";

interface Props {
  snap: Snapshot;
  setInput: (i: number, v: boolean) => void;
  setAnalog: (i: number, v: number) => void;
}

export function IOPanel({ snap, setInput, setAnalog }: Props) {
  // false = botão (momentâneo) | true = chave (mantém a posição)
  const [latched, setLatched] = useState<boolean[]>(() => snap.inputs.map(() => false));

  const toggleMode = (i: number) => {
    setLatched((l) => l.map((v, k) => (k === i ? !v : v)));
    setInput(i, false);
  };

  return (
    <section className="panel">
      <h2>Entradas</h2>
      <div className="io-list">
        {snap.inputs.map((on, i) => (
          <div className="io-row" key={i}>
            <span className={on ? "lamp on" : "lamp"} />
            <button
              className={on ? "push active" : "push"}
              onPointerDown={(e) => {
                if (latched[i]) return;
                e.currentTarget.setPointerCapture(e.pointerId);
                setInput(i, true);
              }}
              onPointerUp={() => { if (!latched[i]) setInput(i, false); }}
              onPointerCancel={() => { if (!latched[i]) setInput(i, false); }}
              onClick={() => { if (latched[i]) setInput(i, !on); }}
            >
              I{i}
            </button>
            <button className="mini" onClick={() => toggleMode(i)} title="Alternar entre botão (momentâneo) e chave (retentiva)">
              {latched[i] ? "chave" : "botão"}
            </button>
          </div>
        ))}
      </div>

      <h2>Analógicas</h2>
      {snap.analog.map((v, i) => (
        <label className="analog" key={i}>
          <span className="mono">A{i}</span>
          <input type="range" min={0} max={4095} value={v} onChange={(e) => setAnalog(i, Number(e.target.value))} />
          <span className="mono value">{v}</span>
        </label>
      ))}

      <h2>Saídas</h2>
      <div className="io-list outs">
        {snap.outputs.map((on, i) => (
          <div className="io-row" key={i}>
            <span className={on ? "lamp out on" : "lamp out"} />
            <span className="mono">Q{i}</span>
          </div>
        ))}
      </div>
    </section>
  );
}
