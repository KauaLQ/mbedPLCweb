import { useEffect, useMemo, useRef, useState, type PointerEvent as ReactPointerEvent } from "react";
import { Header } from "./components/Header";
import { Inspector, type Selection } from "./components/Inspector";
import { IOPanel } from "./components/IOPanel";
import { MonitorPanel } from "./components/MonitorPanel";
import { RungView } from "./components/RungView";
import { defaultProgram } from "./editor/defaultProgram";
import { ELEM_W, measure } from "./editor/layout";
import { loadProgram, saveProgram } from "./editor/storage";
import {
  addOutput, addRung, collectIds, describeNode, findNode, moveNode, moveRung, removeRung,
  type DropHint, type DropZone,
} from "./editor/tree";
import { usePlc } from "./hooks/usePlc";
import type { Program } from "./engine/types";

type Theme = "light" | "dark";
const NO_IDS = new Set<string>();
const ZONE_TEXT: Record<DropZone, string> = {
  before: "em série, antes de",
  after: "em série, depois de",
  above: "em paralelo com",
  below: "em paralelo com",
};

function initialTheme(): Theme {
  try {
    const t = localStorage.getItem("mbedplc.theme");
    if (t === "light" || t === "dark") return t;
  } catch { /* ignora */ }
  return window.matchMedia?.("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

// Descobre qual elemento está sob o ponteiro e em qual região dele (esquerda/direita = série, cima/baixo = paralelo)
function hitTest(x: number, y: number, fromRungId: string, dragId: string, program: Program): DropHint | null {
  const el = document.elementFromPoint(x, y)?.closest("[data-drop]");
  if (!el) return null;
  const rungId = el.getAttribute("data-rung")!;
  const nodeId = el.getAttribute("data-node")!;

  const src = program.lines.find((r) => r.id === fromRungId);
  const moving = src && findNode(src.logic, dragId);
  if (!moving) return null;
  if (rungId === fromRungId && findNode(moving, nodeId)) return null;   // não dá para soltar dentro de si mesmo

  const r = el.getBoundingClientRect();
  const fx = (x - r.left) / r.width;
  const fy = (y - r.top) / r.height;
  const zone: DropZone = fx < 0.28 ? "before" : fx > 0.72 ? "after" : fy < 0.5 ? "above" : "below";
  return { rungId, nodeId, zone };
}

export default function App() {
  const [program, setProgram] = useState<Program>(() => loadProgram() ?? defaultProgram());
  const [sel, setSel] = useState<Selection>(null);
  const [theme, setTheme] = useState<Theme>(initialTheme);
  const [drag, setDrag] = useState<{ rungId: string; id: string; x: number; y: number } | null>(null);
  const [hint, setHint] = useState<DropHint | null>(null);
  const { snap, running, run, stop, setInput, setAnalog } = usePlc(program);

  const programRef = useRef(program);
  programRef.current = program;

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

  // ---------------- arrastar e soltar ----------------
  const startDrag = (rungId: string, id: string, e: ReactPointerEvent) => {
    if (e.button !== 0) return;
    const sx = e.clientX;
    const sy = e.clientY;
    let active = false;     // só vira arraste depois de mover alguns pixels (clique continua selecionando)

    const cleanup = () => {
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
      window.removeEventListener("pointercancel", cleanup);
      document.body.classList.remove("dragging");
      setDrag(null);
      setHint(null);
    };
    const move = (ev: PointerEvent) => {
      if (!active) {
        if (Math.hypot(ev.clientX - sx, ev.clientY - sy) < 5) return;
        active = true;
        document.body.classList.add("dragging");
      }
      setDrag({ rungId, id, x: ev.clientX, y: ev.clientY });
      setHint(hitTest(ev.clientX, ev.clientY, rungId, id, programRef.current));
    };
    const up = (ev: PointerEvent) => {
      const target = active ? hitTest(ev.clientX, ev.clientY, rungId, id, programRef.current) : null;
      cleanup();
      if (!target) return;
      const src = programRef.current.lines.find((r) => r.id === rungId);
      const moved = src && findNode(src.logic, id);
      setProgram((p) => moveNode(p, { rungId, nodeId: id }, target));
      setSel(moved && moved.type !== "OP" ? { rungId: target.rungId, kind: "node", id } : null);
    };

    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
    window.addEventListener("pointercancel", cleanup);
  };

  // Dados do "fantasma" que acompanha o cursor
  let dragIds = NO_IDS;
  let ghost: { x: number; y: number; label: string; text: string } | null = null;
  if (drag) {
    const rung = program.lines.find((r) => r.id === drag.rungId);
    const node = rung && findNode(rung.logic, drag.id);
    if (node) {
      dragIds = new Set(collectIds(node));
      const targetRung = hint && program.lines.find((r) => r.id === hint.rungId);
      const target = targetRung && hint && findNode(targetRung.logic, hint.nodeId);
      ghost = {
        x: drag.x,
        y: drag.y,
        label: describeNode(node),
        text: hint && target ? `${ZONE_TEXT[hint.zone]} ${describeNode(target)}` : "solte sobre um elemento",
      };
    }
  }

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
              dragIds={dragIds}
              hint={hint && hint.rungId === rung.id ? hint : null}
              onDragStart={(id, e) => startDrag(rung.id, id, e)}
              onSelectNode={(id) => setSel({ rungId: rung.id, kind: "node", id })}
              onSelectOutput={(id) => setSel({ rungId: rung.id, kind: "output", id })}
              onMove={(dir) => setProgram((p) => moveRung(p, rung.id, dir))}
              onRemove={() => { setProgram((p) => removeRung(p, rung.id)); setSel(null); }}
              onAddOutput={() => {
                const [p, id] = addOutput(program, rung.id);
                setProgram(p);
                setSel({ rungId: rung.id, kind: "output", id });
              }}
            />
          ))}

          <button className="add-rung" onClick={() => setProgram((p) => addRung(p))}>+ Nova linha</button>
        </section>

        <aside className="side">
          <Inspector program={program} sel={sel} onProgram={setProgram} onSelect={setSel} />
        </aside>
      </main>

      {ghost && (
        <div className="drag-ghost" style={{ left: ghost.x + 14, top: ghost.y + 14 }}>
          <b className="mono">{ghost.label}</b>
          <span>{ghost.text}</span>
        </div>
      )}
    </div>
  );
}
