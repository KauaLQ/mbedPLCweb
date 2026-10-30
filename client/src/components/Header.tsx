interface Props {
  running: boolean;
  theme: "light" | "dark";
  onRun: () => void;
  onStop: () => void;
  onExample: () => void;
  onClear: () => void;
  onToggleTheme: () => void;
}

export function Header(p: Props) {
  return (
    <header className="topbar">
      <div className="brand">
        <img className="logo logo-light" src="/logo-mark.png" alt="mbedPLC" />
        <img className="logo logo-dark" src="/logo-mark-dark.png" alt="mbedPLC" />
        <span className="brand-sub">Simulador Ladder</span>
      </div>

      <div className="topbar-actions">
        <span className={p.running ? "status run" : "status stop"}>{p.running ? "RUN" : "STOP"}</span>
        <button className="primary" onClick={p.onRun} disabled={p.running}>▶ Executar</button>
        <button onClick={p.onStop} disabled={!p.running}>■ Parar</button>
        <span className="sep" />
        <button onClick={p.onExample}>Exemplo</button>
        <button onClick={p.onClear}>Limpar</button>
        <button onClick={p.onToggleTheme} title="Alternar tema claro/escuro">{p.theme === "dark" ? "☀ Claro" : "☾ Escuro"}</button>
      </div>
    </header>
  );
}
