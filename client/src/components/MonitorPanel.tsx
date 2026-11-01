import type { Snapshot } from "../engine/plc";
import { fmtSec } from "../engine/format";

const flag = (on: boolean, label: string) => <span className={on ? "flag on" : "flag"}>{label}</span>;

export function MonitorPanel({ snap }: { snap: Snapshot }) {
  return (
    <section className="panel">
      <h2>Memórias</h2>
      <div className="chips">
        {snap.memory.map((on, i) => (
          <span key={i} className={on ? "chip on" : "chip"}>M{i}</span>
        ))}
      </div>

      <h2>Timers</h2>
      <table className="mon">
        <tbody>
          {snap.timers.map((t, i) => (
            <tr key={i}>
              <td className="mono">T{i}</td>
              <td>{flag(t.EN, "EN")}{flag(t.TT, "TT")}{flag(t.DN, "DN")}</td>
              <td className="mono num">{fmtSec(t.accum)} / {fmtSec(t.preset)} s</td>
            </tr>
          ))}
        </tbody>
      </table>

      <h2>Contadores</h2>
      <table className="mon">
        <tbody>
          {snap.counters.map((c, i) => (
            <tr key={i}>
              <td className="mono">C{i}</td>
              <td>{flag(c.DN, "DN")}</td>
              <td className="mono num">{c.ACC} / {c.PRE}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </section>
  );
}
