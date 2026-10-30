import { useEffect, useMemo, useState } from "react";
import { Header } from "./components/Header";
import { Inspector, type Selection } from "./components/Inspector";
import { IOPanel } from "./components/IOPanel";
import { MonitorPanel } from "./components/MonitorPanel";
import { RungView } from "./components/RungView";
import { defaultProgram } from "./editor/defaultProgram";
import { ELEM_W, measure } from "./editor/layout";
import { loadProgram, saveProgram } from "./editor/storage";
import { addOutput, addRung, moveRung, removeRung } from "./editor/tree";
import { usePlc } from "./hooks/usePlc";
import type { Program } from "./engine/types";

type Theme = "light" | "dark";

function initialTheme(): Theme {
  try {
    const t = localStorage.getItem("mbedplc.theme");
    if (t === "light" || t === "dark") return t;
  } catch { /* ignora */ }
  return window.matchMedia?.("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

export default function App() {
  const [program, setProgram] = useState<Program>(() => loadProgram() ?? defaultProgram());
  const [sel, setSel] = useState<Selection>(null);
  const [theme, setTheme] = useState<Theme>(initialTheme);
  const { snap, running, run, stop, setInput, setAnalog } = usePlc(program);

  useEffect(() => saveProgram(program), [program]);
  useEffect(() => {
    document.documentElement.dataset.theme = theme;
    try { localStorage.setItem("mbedplc.theme", theme); } catch { /* ignora */ }
  }, [theme]);

  // Alinha as saídas de todas as linhas na mesma coluna
  const maxLogicW = useMemo(
    () => Math.max(ELEM_W, ...program.lines.map((r) => measure(r.logic).w)),
    [program],
  );

  const select = (s: Selection) => setSel(s);

  return (
    <div className="app">
      <Header
        running={running}
        theme={theme}
        onRun={run}
        onStop={stop}
        onExample={() => { setProgram(defaultProgram()); setSel(null); }}
        onClear={() => { setProgram({ lines: [] }); setSel(null); }}
        onToggleTheme={() => setTheme((t) => (t === "dark" ? "light" : "dark"))}
      />

      <main className="layout">
        <aside className="side">
          <IOPanel snap={snap} setInput={setInput} setAnalog={setAnalog} />
          <MonitorPanel snap={snap} />
        </aside>

        <section className="canvas">
          {program.lines.length === 0 && <p className="hint empty">Nenhuma linha. Use “+ Nova linha” para começar.</p>}

          {program.lines.map((rung, i) => (
            <RungView
              key={rung.id}
              rung={rung}
              index={i}
              total={program.lines.length}
              snap={snap}
              selectedId={sel && sel.rungId === rung.id ? sel.id : null}
              maxLogicW={maxLogicW}
              onSelectNode={(id) => select({ rungId: rung.id, kind: "node", id })}
              onSelectOutput={(id) => select({ rungId: rung.id, kind: "output", id })}
              onMove={(dir) => setProgram((p) => moveRung(p, rung.id, dir))}
              onRemove={() => { setProgram((p) => removeRung(p, rung.id)); setSel(null); }}
              onAddOutput={() => {
                const [p, id] = addOutput(program, rung.id);
                setProgram(p);
                select({ rungId: rung.id, kind: "output", id });
              }}
            />
          ))}

          <button className="add-rung" onClick={() => setProgram((p) => addRung(p))}>+ Nova linha</button>
        </section>

        <aside className="side">
          <Inspector program={program} sel={sel} onProgram={setProgram} onSelect={select} />
        </aside>
      </main>
    </div>
  );
}
