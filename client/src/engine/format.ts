// ms -> segundos com 1 casa (muda a cada 100 ms: evita a tela "saltitar")
export const fmtSec = (ms: number) => (ms / 1000).toFixed(1);
