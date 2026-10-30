import type { Snapshot } from "../engine/plc";
import type { Rung } from "../engine/types";
import { ELEM_H, OUT_W, RAIL_X, WIRE_Y, measure } from "../editor/layout";
import { drawNode, drawOutput } from "./LadderNode";

interface Props {
  rung: Rung;
  index: number;
  total: number;
  snap: Snapshot;
  selectedId: string | null;
  maxLogicW: number;          // largura da maior lógica: alinha as saídas de todas as linhas
  onSelectNode: (id: string) => void;
  onSelectOutput: (id: string) => void;
  onMove: (dir: -1 | 1) => void;
  onRemove: () => void;
  onAddOutput: () => void;
}

export function RungView(p: Props) {
  const { rung, snap } = p;
  const live = snap.scans > 0;                       // parado: nenhuma barra energizada
  const wire = (on: boolean) => (on ? "wire on" : "wire");

  const logicSize = measure(rung.logic);
  const H = Math.max(logicSize.h, rung.outputs.length * ELEM_H, ELEM_H);
  const outX = RAIL_X + p.maxLogicW;                 // início da coluna de saídas
  const symX = outX + 18;
  const railR = symX + OUT_W + 18;
  const W = railR + RAIL_X;

  const { el, out } = drawNode(rung.logic, RAIL_X, 0, live, {
    results: snap.results,
    selectedId: p.selectedId,
    onSelect: p.onSelectNode,
  });

  const lastY = WIRE_Y + (Math.max(rung.outputs.length, 1) - 1) * ELEM_H;

  return (
    <section className={out ? "rung energized" : "rung"}>
      <header className="rung-head">
        <span className="rung-title">Linha {String(p.index + 1).padStart(2, "0")}</span>
        <span className="spacer" />
        <button className="mini" onClick={p.onAddOutput} title="Adicionar instrução de saída">+ saída</button>
        <button className="mini" onClick={() => p.onMove(-1)} disabled={p.index === 0} title="Mover para cima">↑</button>
        <button className="mini" onClick={() => p.onMove(1)} disabled={p.index === p.total - 1} title="Mover para baixo">↓</button>
        <button className="mini danger" onClick={p.onRemove} title="Excluir linha">✕</button>
      </header>

      <div className="rung-scroll">
        <svg width={W} height={H + 4} viewBox={`0 0 ${W} ${H + 4}`}>
          <line className={live ? "rail on" : "rail"} x1={RAIL_X} y1={0} x2={RAIL_X} y2={H} />
          <line className={live ? "rail on" : "rail"} x1={railR} y1={0} x2={railR} y2={H} />

          {el}
          <line className={wire(out)} x1={RAIL_X + logicSize.w} y1={WIRE_Y} x2={outX} y2={WIRE_Y} />

          {rung.outputs.length === 0 && (
            <line className={wire(out)} x1={outX} y1={WIRE_Y} x2={railR} y2={WIRE_Y} />
          )}
          {rung.outputs.length > 0 && (
            <line className={wire(out)} x1={outX} y1={WIRE_Y} x2={outX} y2={lastY} />
          )}
          {rung.outputs.map((o, i) => {
            const wy = i * ELEM_H + WIRE_Y;
            return (
              <g key={o.id}>
                <line className={wire(out)} x1={outX} y1={wy} x2={symX} y2={wy} />
                <line className={wire(out)} x1={symX + OUT_W} y1={wy} x2={railR} y2={wy} />
                {drawOutput(o, symX, i * ELEM_H, out, snap, p.selectedId === o.id, p.onSelectOutput)}
              </g>
            );
          })}
        </svg>
      </div>
    </section>
  );
}
