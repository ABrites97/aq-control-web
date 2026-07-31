"use client";

import { useEffect, useState, useCallback, useRef } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
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

  return (
    <div>
      <h1>🔥 AQ-CONTROL</h1>

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
        <div style={{ height: "256px", width: "100%" }}>
          <ResponsiveContainer>
            <LineChart data={historico}>
              <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
              <XAxis
                dataKey="criadoEm"
                tickFormatter={(v) =>
                  new Date(v).toLocaleTimeString("pt-PT", {
                    hour: "2-digit",
                    minute: "2-digit",
                  })
                }
                stroke="#cbd5e1"
                fontSize={12}
              />
              <YAxis stroke="#cbd5e1" fontSize={12} />
              <Tooltip
                labelFormatter={(v) => new Date(v).toLocaleString("pt-PT")}
                contentStyle={{ background: "#1e293b", border: "none", color: "white" }}
              />
              <Legend />
              <Line type="monotone" dataKey="tempCaldeira" name="Caldeira" stroke="#ff9800" dot={false} strokeWidth={2} />
              <Line type="monotone" dataKey="tempAQS" name="AQS" stroke="#00e676" dot={false} strokeWidth={2} />
            </LineChart>
          </ResponsiveContainer>
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
