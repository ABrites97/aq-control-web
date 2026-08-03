"use client";

import { useEffect, useState, useCallback, useRef } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  ReferenceArea,
} from "recharts";

type Leitura = {
  id: number;
  criadoEm: string;
  tempCaldeira: number;
  tempAQS: number;
  rele1Ligado: boolean;
  rele2Ligado: boolean;
  rele3Ligado: boolean;
  modo: string;
  radiadoresPausados: boolean;
};

const MODOS = ["ON", "INVERNO", "VERAO", "OFF"];

// Agrupa leituras consecutivas em que um rele esteve ligado, em intervalos [inicio, fim]
function gerarIntervalos(
  dados: (Leitura & { ts: number })[],
  campo: "rele2Ligado" | "rele3Ligado"
): { inicio: number; fim: number }[] {
  const intervalos: { inicio: number; fim: number }[] = [];
  let inicioAtual: number | null = null;

  dados.forEach((d) => {
    if (d[campo] && inicioAtual === null) {
      inicioAtual = d.ts;
    }
    if (!d[campo] && inicioAtual !== null) {
      intervalos.push({ inicio: inicioAtual, fim: d.ts });
      inicioAtual = null;
    }
  });

  if (inicioAtual !== null && dados.length > 0) {
    intervalos.push({ inicio: inicioAtual, fim: dados[dados.length - 1].ts });
  }

  return intervalos;
}
function gerarTicksHoras(inicio: number, fim: number): number[] {
  const ticks: number[] = [];
  const d = new Date(fim);
  d.setMinutes(0, 0, 0);
  if (d.getHours() % 2 !== 0) d.setHours(d.getHours() - 1);
  let t = d.getTime();
  while (t >= inicio) {
    ticks.push(t);
    t -= 2 * 60 * 60 * 1000;
  }
  return ticks.reverse();
}

export default function Home() {
  const [ultima, setUltima] = useState<Leitura | null>(null);
  const [historico, setHistorico] = useState<Leitura[]>([]);
  const [aEnviar, setAEnviar] = useState(false);

  // A "Data e Hora" so atualiza a cada 30s - o resto (reles, temperaturas, modo)
  // continua a atualizar a cada 5s. Usamos uma ref para guardar sempre a leitura
  // mais recente, sem forcar a caixa da hora a mudar antes dos 30s passarem.
  const ultimaRef = useRef<Leitura | null>(null);
  const [horaExibidaEm, setHoraExibidaEm] = useState<string | null>(null);

  const carregar = useCallback(async () => {
    try {
      const res = await fetch("/api/leituras", { cache: "no-store" });
      const dados = await res.json();
      setUltima(dados.ultima);
      setHistorico(dados.historico);
      ultimaRef.current = dados.ultima;
    } catch (e) {
      console.error(e);
    }
  }, []);

  useEffect(() => {
    carregar();
    const t = setInterval(carregar, 5000);
    return () => clearInterval(t);
  }, [carregar]);

  useEffect(() => {
    const atualizarHora = () => {
      if (ultimaRef.current) setHoraExibidaEm(ultimaRef.current.criadoEm);
    };
    atualizarHora(); // mostra logo a primeira leitura, sem esperar 30s
    const t = setInterval(atualizarHora, 30000);
    return () => clearInterval(t);
  }, []);

  async function enviarComando(tipo: string, valor: string) {
    setAEnviar(true);
    try {
      await fetch("/api/comandos", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ tipo, valor }),
      });
    } finally {
      setTimeout(() => setAEnviar(false), 1500);
    }
  }

  if (!ultima) {
    return <div>A carregar dados...</div>;
  }

  const dadosGrafico = historico.map((h) => ({ ...h, ts: new Date(h.criadoEm).getTime() }));
  const intervalosBombaCaldeira = gerarIntervalos(dadosGrafico, "rele2Ligado");
  const intervalosBombaAquecimento = gerarIntervalos(dadosGrafico, "rele3Ligado");

  return (
    <div>
      <h1>🔥 AQ-CONTROL 🚿</h1>

      <div className="card">
        <div className="titulo">🕒 Data e Hora</div>
        <div className="valor">
          {horaExibidaEm &&
            new Date(horaExibidaEm).toLocaleTimeString("pt-PT", {
              hour: "2-digit",
              minute: "2-digit",
            })}
        </div>
        <div className="data">
          {horaExibidaEm && new Date(horaExibidaEm).toLocaleDateString("pt-PT")}
        </div>
      </div>

      <div className="card">
        <div className="titulo">⚙️ Modo Caldeira</div>
        <div className="modos">
          {MODOS.map((m) => (
            <button
              key={m}
              className={`aq-btn ${ultima.modo === m ? "ativo" : ""}`}
              onClick={() => enviarComando("modo", m)}
            >
              {m === "VERAO" ? "VERÃO" : m}
            </button>
          ))}
        </div>
        {aEnviar && (
          <div style={{ fontSize: "14px", color: "#ff9800", marginTop: "10px" }}>
            A enviar comando...
          </div>
        )}
      </div>

      <div className="card">
        <div className="titulo">🌡 Temperaturas</div>
        <div className="linha">
          <span>🔥 Caldeira:</span>
          <span>{ultima.tempCaldeira.toFixed(1)} °C</span>
        </div>
        <div className="linha">
          <span>🚿 AQS:</span>
          <span>{ultima.tempAQS.toFixed(1)} °C</span>
        </div>
      </div>

      <div className="card">
        <div className="titulo">⚡ Saídas</div>
        <Linha label="Ordem Caldeira:" ligado={ultima.rele1Ligado} />
        <Linha label="Bomba Caldeira:" ligado={ultima.rele2Ligado} />
        <Linha label="Bomba Aquecimento:" ligado={ultima.rele3Ligado} />

        <button
          className={`aq-btn ${ultima.radiadoresPausados ? "ativo" : ""}`}
          style={{ width: "90%" }}
          onClick={() =>
            enviarComando("radiadores_pausa", ultima.radiadoresPausados ? "0" : "1")
          }
        >
          {ultima.radiadoresPausados ? "▶️ Retomar Radiadores" : "⏸️ Pausar Radiadores"}
        </button>
      </div>

      <div className="card">
        <div className="titulo">📈 Histórico de Temperaturas (24h)</div>
        <div style={{ height: "256px", width: "100%", display: "flex", justifyContent: "center" }}>
          <ResponsiveContainer>
            <LineChart
              data={dadosGrafico}
              margin={{ top: 10, right: 15, left: -5, bottom: 0 }}
            >
              <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
              <XAxis
                dataKey="ts"
                type="number"
                domain={[Date.now() - 24 * 60 * 60 * 1000, Date.now()]}
                ticks={gerarTicksHoras(Date.now() - 24 * 60 * 60 * 1000, Date.now())}
                interval={0}
                tickFormatter={(v) =>
                  new Date(v).toLocaleTimeString("pt-PT", {
                    hour: "2-digit",
                    minute: "2-digit",
                  })
                }
                stroke="#cbd5e1"
                fontSize={11}
              />
              <YAxis
                domain={[10, 100]}
                ticks={[10, 20, 30, 40, 50, 60, 70, 80, 90, 100]}
                stroke="#cbd5e1"
                fontSize={12}
                width={42}
                allowDataOverflow
              />
              <Tooltip
                labelFormatter={(v) => new Date(v).toLocaleString("pt-PT")}
                contentStyle={{ background: "#1e293b", border: "none", color: "white" }}
              />

              {intervalosBombaCaldeira.map((iv, idx) => (
                <ReferenceArea
                  key={"bc" + idx}
                  x1={iv.inicio}
                  x2={iv.fim}
                  strokeOpacity={0}
                  fill="#ef4444"
                  fillOpacity={0.22}
                />
              ))}
              {intervalosBombaAquecimento.map((iv, idx) => (
                <ReferenceArea
                  key={"ba" + idx}
                  x1={iv.inicio}
                  x2={iv.fim}
                  strokeOpacity={0}
                  fill="#22d3ee"
                  fillOpacity={0.22}
                />
              ))}

              <Line type="monotone" dataKey="tempCaldeira" name="Caldeira" stroke="#ef4444" dot={false} strokeWidth={2} />
              <Line type="monotone" dataKey="tempAQS" name="AQS" stroke="#38bdf8" dot={false} strokeWidth={2} />
            </LineChart>
          </ResponsiveContainer>
        </div>

        <div style={{ display: "flex", justifyContent: "center", alignItems: "center", gap: "10px", marginTop: "10px", fontSize: "11px", flexWrap: "nowrap", overflowX: "auto", whiteSpace: "nowrap" }}>
          <span style={{ display: "inline-flex", alignItems: "center", gap: "4px" }}>
            <span style={{ width: "12px", height: "2px", background: "#ef4444", display: "inline-block" }} />
            Caldeira
          </span>
          <span style={{ display: "inline-flex", alignItems: "center", gap: "4px" }}>
            <span style={{ width: "12px", height: "2px", background: "#38bdf8", display: "inline-block" }} />
            AQS
          </span>
          <span style={{ display: "inline-flex", alignItems: "center", gap: "4px" }}>
            <span style={{ width: "10px", height: "10px", background: "#ef4444", opacity: 0.5, borderRadius: "2px", display: "inline-block" }} />
            B. Caldeira
          </span>
          <span style={{ display: "inline-flex", alignItems: "center", gap: "4px" }}>
            <span style={{ width: "10px", height: "10px", background: "#22d3ee", opacity: 0.5, borderRadius: "2px", display: "inline-block" }} />
            B. Aquecimento
          </span>
        </div>
      </div>

      <div className="card">
        <a
          href="http://192.168.68.200"
          className="aq-btn"
          style={{ display: "inline-block", textDecoration: "none", width: "90%" }}
        >
          ⚙️ Modo Programador
        </a>
      </div>
    </div>
  );
}

function Linha({ label, ligado }: { label: string; ligado: boolean }) {
  return (
    <div className="linha">
      <span>{label}</span>
      <span className={ligado ? "ligado" : "desligado"}>
        {ligado ? "🟢 ON" : "🔴 OFF"}
      </span>
    </div>
  );
}
