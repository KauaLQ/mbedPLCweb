import type { PointerEvent as ReactPointerEvent, ReactNode } from "react";
import type { Snapshot } from "../engine/plc";
import { fmtSec } from "../engine/format";
import type { LogicNode, Output } from "../engine/types";
import type { DropZone } from "../editor/tree";
import { ELEM_H, ELEM_W, OUT_W, PAD, WIRE_Y, measure } from "../editor/layout";

export interface DrawCtx {
  rungId: string;
  results: Map<string, boolean>;                       // resultado de cada nó no último scan
  selectedId: string | null;
  dragIds: Set<string>;                                // nós que estão sendo arrastados
  hint: { nodeId: string; zone: DropZone } | null;     // alvo atual da soltura
  onSelect: (id: string) => void;
  onDragStart: (id: string, e: ReactPointerEvent) => void;
}

const wire = (on: boolean) => (on ? "wire on" : "wire");
type Leaf = Exclude<LogicNode, { type: "OP" }>;

// Marca onde o elemento arrastado vai entrar
function dropMark(zone: DropZone) {
  switch (zone) {
    case "before": return <rect className="drop-mark" x={0} y={6} width={5} height={ELEM_H - 12} rx={2} />;
    case "after":  return <rect className="drop-mark" x={ELEM_W - 5} y={6} width={5} height={ELEM_H - 12} rx={2} />;
    case "above":  return <rect className="drop-mark" x={14} y={3} width={ELEM_W - 28} height={5} rx={2} />;
    case "below":  return <rect className="drop-mark" x={14} y={ELEM_H - 8} width={ELEM_W - 28} height={5} rx={2} />;
  }
}

// ---------------- elementos de lógica (contatos) ----------------
function drawLeaf(n: Leaf, x: number, y: number, powerIn: boolean, ctx: DrawCtx) {
  const res = ctx.results.get(n.id) ?? false;
  const out = powerIn && res;
  const cx = ELEM_W / 2;
  const sym = res ? "sym on" : "sym";
  const box = res ? "box on" : "box";

  let half = 9;
  let shape: ReactNode;
  let top: string;
  let bottom = "";
  let bottomClass = "sub";

  if (n.type === "CONTACT") {
    top = n.pin;
    shape = (
      <>
        <line className={sym} x1={cx - 9} y1={WIRE_Y - 14} x2={cx - 9} y2={WIRE_Y + 14} />
        <line className={sym} x1={cx + 9} y1={WIRE_Y - 14} x2={cx + 9} y2={WIRE_Y + 14} />
        {n.mode === "NC" && <line className={sym} x1={cx - 7} y1={WIRE_Y + 11} x2={cx + 7} y2={WIRE_Y - 11} />}
      </>
    );
  } else if (n.type === "COMPARE") {
    half = 28;
    top = n.left;
    bottom = n.right;
    bottomClass = "tag";
    shape = (
      <>
        <rect className={box} x={cx - 28} y={WIRE_Y - 15} width={56} height={30} rx={2} />
        <text className="op" x={cx} y={WIRE_Y + 5} textAnchor="middle">{n.op}</text>
      </>
    );
  } else {
    half = 18;
    top = n.pin;
    bottom = n.type === "R_TRIG" ? "borda ↑" : "borda ↓";
    shape = (
      <>
        <rect className={box} x={cx - 18} y={WIRE_Y - 14} width={36} height={28} rx={2} />
        <text className="op" x={cx} y={WIRE_Y + 5} textAnchor="middle">{n.type === "R_TRIG" ? "P" : "N"}</text>
      </>
    );
  }

  const hint = ctx.hint && ctx.hint.nodeId === n.id ? ctx.hint.zone : null;

  return {
    out,
    el: (
      <g
        key={n.id}
        transform={`translate(${x},${y})`}
        className={ctx.dragIds.has(n.id) ? "node dragging" : "node"}
        onClick={() => ctx.onSelect(n.id)}
        onPointerDown={(e) => ctx.onDragStart(n.id, e)}
      >
        <rect
          className={ctx.selectedId === n.id ? "hit selected" : "hit"}
          data-drop="1" data-rung={ctx.rungId} data-node={n.id}
          x={3} y={3} width={ELEM_W - 6} height={ELEM_H - 6} rx={3}
        />
        <line className={wire(powerIn)} x1={0} y1={WIRE_Y} x2={cx - half} y2={WIRE_Y} />
        <line className={wire(out)} x1={cx + half} y1={WIRE_Y} x2={ELEM_W} y2={WIRE_Y} />
        {shape}
        <text className="tag" x={cx} y={20} textAnchor="middle">{top}</text>
        {bottom && <text className={bottomClass} x={cx} y={WIRE_Y + 30} textAnchor="middle">{bottom}</text>}
        {hint && dropMark(hint)}
      </g>
    ),
  };
}

// ---------------- árvore: AND = série, OR = paralelo ----------------
export function drawNode(n: LogicNode, x: number, y: number, powerIn: boolean, ctx: DrawCtx): { el: ReactNode; out: boolean } {
  if (n.type !== "OP") return drawLeaf(n, x, y, powerIn, ctx);

  if (n.op === "AND") {
    const els: ReactNode[] = [];
    let cx = x;
    let p = powerIn;
    for (const c of n.nodes) {
      const r = drawNode(c, cx, y, p, ctx);
      els.push(r.el);
      p = r.out;
      cx += measure(c).w;
    }
    return { el: <g key={n.id}>{els}</g>, out: p };
  }

  const m = measure(n);
  const els: ReactNode[] = [];
  const handles: ReactNode[] = [];
  let oy = y;
  let anyOut = false;
  let firstY = 0;
  let lastY = 0;
  n.nodes.forEach((c, i) => {
    const cm = measure(c);
    const r = drawNode(c, x + PAD, oy, powerIn, ctx);
    const wy = oy + WIRE_Y;
    if (i === 0) firstY = wy;
    lastY = wy;
    els.push(r.el);
    els.push(<line key={`${c.id}-i`} className={wire(powerIn)} x1={x} y1={wy} x2={x + PAD} y2={wy} />);
    els.push(<line key={`${c.id}-o`} className={wire(r.out)} x1={x + PAD + cm.w} y1={wy} x2={x + m.w} y2={wy} />);
    // alça do ramo: arrastar a conexão move o ramo inteiro (elemento ou sequência)
    handles.push(
      <line key={`${c.id}-h`} className="handle" x1={x} y1={wy} x2={x + PAD} y2={wy} onPointerDown={(e) => ctx.onDragStart(c.id, e)}>
        <title>Arraste para mover este ramo</title>
      </line>,
    );
    anyOut = anyOut || r.out;
    oy += cm.h;
  });
  els.push(<line key="vl" className={wire(powerIn)} x1={x} y1={firstY} x2={x} y2={lastY} />);
  els.push(<line key="vr" className={wire(anyOut)} x1={x + m.w} y1={firstY} x2={x + m.w} y2={lastY} />);
  // alças das barras verticais: arrastar move o bloco paralelo inteiro
  for (const bx of [x, x + m.w]) {
    handles.push(
      <line key={`bar-${bx}`} className="handle" x1={bx} y1={firstY} x2={bx} y2={lastY} onPointerDown={(e) => ctx.onDragStart(n.id, e)}>
        <title>Arraste para mover o bloco paralelo</title>
      </line>,
    );
  }
  return { el: <g key={n.id}>{els}{handles}</g>, out: anyOut };
}

// ---------------- instruções de saída ----------------
const OUT_BOX_W = 104;

export function drawOutput(
  o: Output, x: number, y: number, powerIn: boolean, snap: Snapshot,
  selected: boolean, onSelect: (id: string) => void,
) {
  const cx = OUT_W / 2;
  const idx = parseInt(o.pin.slice(1), 10);
  let shape: ReactNode;
  let half: number;

  if (o.type === "COIL" || o.type === "SET" || o.type === "RESET") {
    half = 14;
    const cls = powerIn ? "sym on" : "sym";
    shape = (
      <>
        <path className={cls} fill="none" d={`M ${cx - 6} ${WIRE_Y - 14} Q ${cx - 19} ${WIRE_Y} ${cx - 6} ${WIRE_Y + 14}`} />
        <path className={cls} fill="none" d={`M ${cx + 6} ${WIRE_Y - 14} Q ${cx + 19} ${WIRE_Y} ${cx + 6} ${WIRE_Y + 14}`} />
        {o.type !== "COIL" && <text className="op" x={cx} y={WIRE_Y + 5} textAnchor="middle">{o.type === "SET" ? "S" : "R"}</text>}
      </>
    );
  } else {
    half = OUT_BOX_W / 2;
    let line1 = `${o.type} ${o.pin}`;
    let line2 = "";
    let on = powerIn;
    if (o.type === "TON" || o.type === "TOF") {
      const t = snap.timers[idx];
      line2 = `${fmtSec(t.accum)} / ${fmtSec(o.preset ?? 0)} s`;
      on = t.DN;
    } else if (o.type === "CTU") {
      const c = snap.counters[idx];
      line2 = `${c.ACC} / ${o.preset ?? 0}`;
      on = c.DN;
    } else {
      line1 = "RES";
      line2 = o.pin;
    }
    shape = (
      <>
        <rect className={on ? "box on" : "box"} x={cx - half} y={WIRE_Y - 22} width={OUT_BOX_W} height={44} rx={2} />
        <text className="op small" x={cx} y={WIRE_Y - 4} textAnchor="middle">{line1}</text>
        <text className="tag" x={cx} y={WIRE_Y + 12} textAnchor="middle">{line2}</text>
      </>
    );
  }

  return (
    <g key={o.id} transform={`translate(${x},${y})`} className="node" onClick={() => onSelect(o.id)}>
      <rect className={selected ? "hit selected" : "hit"} x={3} y={3} width={OUT_W - 6} height={ELEM_H - 6} rx={3} />
      <line className={wire(powerIn)} x1={0} y1={WIRE_Y} x2={cx - half} y2={WIRE_Y} />
      <line className={wire(powerIn)} x1={cx + half} y1={WIRE_Y} x2={OUT_W} y2={WIRE_Y} />
      {shape}
      {(o.type === "COIL" || o.type === "SET" || o.type === "RESET") && <text className="tag" x={cx} y={20} textAnchor="middle">{o.pin}</text>}
    </g>
  );
}
