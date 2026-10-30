import { TAGS, needsPreset, pinsForOutput } from "../engine/tags";
import type { CmpOp, LogicNode, Output, OutputType, Program } from "../engine/types";
import {
  addNode, convertNode, deleteNode, deleteOutput, editNode, editOutput, findNode,
} from "../editor/tree";

export type Selection = { rungId: string; kind: "node" | "output"; id: string } | null;

interface Props {
  program: Program;
  sel: Selection;
  onProgram: (p: Program) => void;
  onSelect: (s: Selection) => void;
}

const OUTPUT_LABELS: [OutputType, string][] = [
  ["COIL", "COIL · Bobina"], ["SET", "SET · Liga e retém"], ["RESET", "RESET · Desliga"],
  ["TON", "TON · Timer com atraso p/ ligar"], ["TOF", "TOF · Timer com atraso p/ desligar"],
  ["CTU", "CTU · Contador crescente"], ["RES_TIMER", "RES_TIMER · Zera timer"], ["RES_COUNTER", "RES_COUNTER · Zera contador"],
];
const CMP_OPS: CmpOp[] = [">", "<", ">=", "<=", "==", "!="];
const isConst = (s: string) => /^-?\d+$/.test(s);

function Select({ value, options, onChange }: { value: string; options: (string | [string, string])[]; onChange: (v: string) => void }) {
  return (
    <select value={value} onChange={(e) => onChange(e.target.value)}>
      {options.map((o) => {
        const [v, label] = typeof o === "string" ? [o, o] : o;
        return <option key={v} value={v}>{label}</option>;
      })}
    </select>
  );
}

const Field = ({ label, children }: { label: string; children: React.ReactNode }) => (
  <label className="field"><span>{label}</span>{children}</label>
);

export function Inspector({ program, sel, onProgram, onSelect }: Props) {
  const rungIndex = sel ? program.lines.findIndex((r) => r.id === sel.rungId) : -1;
  const rung = rungIndex >= 0 ? program.lines[rungIndex] : null;

  if (!sel || !rung) {
    return (
      <section className="panel">
        <h2>Inspetor</h2>
        <p className="hint">
          Clique em um contato ou em uma instrução de saída para editar.
          Use <b>+ Série</b> e <b>+ Paralelo</b> para montar a lógica da linha.
        </p>
      </section>
    );
  }

  // ---------------- saída selecionada ----------------
  if (sel.kind === "output") {
    const o = rung.outputs.find((x) => x.id === sel.id);
    if (!o) return null;
    const patch = (p: Partial<Output>) => onProgram(editOutput(program, rung.id, o.id, p));
    return (
      <section className="panel">
        <h2>Saída · Linha {rungIndex + 1}</h2>
        <Field label="Instrução"><Select value={o.type} options={OUTPUT_LABELS} onChange={(v) => patch({ type: v as OutputType })} /></Field>
        <Field label="Tag"><Select value={o.pin} options={pinsForOutput(o.type)} onChange={(v) => patch({ pin: v })} /></Field>
        {needsPreset(o.type) && (
          <Field label={o.type === "CTU" ? "Preset (contagens)" : "Preset (ms)"}>
            <input type="number" min={0} value={o.preset ?? 0} onChange={(e) => patch({ preset: Math.max(0, parseInt(e.target.value, 10) || 0) })} />
          </Field>
        )}
        <div className="actions">
          <button className="danger" onClick={() => { onProgram(deleteOutput(program, rung.id, o.id)); onSelect(null); }}>Excluir</button>
        </div>
      </section>
    );
  }

  // ---------------- elemento de lógica selecionado ----------------
  const node = findNode(rung.logic, sel.id);
  if (!node || node.type === "OP") return null;
  const edit = (fn: (n: LogicNode) => LogicNode) => onProgram(editNode(program, rung.id, node.id, fn));
  const add = (op: "AND" | "OR") => {
    const [p, newId] = addNode(program, rung.id, node.id, op);
    onProgram(p);
    onSelect({ rungId: rung.id, kind: "node", id: newId });
  };

  return (
    <section className="panel">
      <h2>Elemento · Linha {rungIndex + 1}</h2>
      <Field label="Tipo">
        <Select
          value={node.type}
          options={[["CONTACT", "Contato"], ["R_TRIG", "Borda de subida (R_TRIG)"], ["F_TRIG", "Borda de descida (F_TRIG)"], ["COMPARE", "Comparação"]]}
          onChange={(v) => edit((n) => convertNode(n, v as "CONTACT" | "R_TRIG" | "F_TRIG" | "COMPARE"))}
        />
      </Field>

      {node.type === "CONTACT" && (
        <>
          <Field label="Tag"><Select value={node.pin} options={TAGS.contact} onChange={(v) => edit((n) => ({ ...n, pin: v }) as LogicNode)} /></Field>
          <Field label="Modo">
            <Select value={node.mode} options={[["NO", "NO · Normalmente aberto"], ["NC", "NC · Normalmente fechado"]]}
              onChange={(v) => edit((n) => ({ ...n, mode: v }) as LogicNode)} />
          </Field>
        </>
      )}

      {(node.type === "R_TRIG" || node.type === "F_TRIG") && (
        <Field label="Tag"><Select value={node.pin} options={TAGS.contact} onChange={(v) => edit((n) => ({ ...n, pin: v }) as LogicNode)} /></Field>
      )}

      {node.type === "COMPARE" && (
        <>
          <Field label="Esquerda"><Select value={node.left} options={TAGS.value} onChange={(v) => edit((n) => ({ ...n, left: v }) as LogicNode)} /></Field>
          <Field label="Operador"><Select value={node.op} options={CMP_OPS} onChange={(v) => edit((n) => ({ ...n, op: v }) as LogicNode)} /></Field>
          <Field label="Direita">
            <Select
              value={isConst(node.right) ? "__const" : node.right}
              options={[["__const", "Constante"], ...TAGS.value]}
              onChange={(v) => edit((n) => ({ ...n, right: v === "__const" ? "0" : v }) as LogicNode)}
            />
          </Field>
          {isConst(node.right) && (
            <Field label="Valor">
              <input type="number" value={node.right} onChange={(e) => edit((n) => ({ ...n, right: String(parseInt(e.target.value, 10) || 0) }) as LogicNode)} />
            </Field>
          )}
        </>
      )}

      <div className="actions">
        <button onClick={() => add("AND")}>+ Série</button>
        <button onClick={() => add("OR")}>+ Paralelo</button>
        <button className="danger" onClick={() => { onProgram(deleteNode(program, rung.id, node.id)); onSelect(null); }}>Excluir</button>
      </div>
    </section>
  );
}
