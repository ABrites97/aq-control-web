"use client";

import { useEffect, useState, useCallback, CSSProperties } from "react";
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

// Valores copiados diretamente do CSS da pagina local do ESP32,
// para as duas paginas ficarem visualmente identicas.
const cardStyle: CSSProperties = {
  background: "#1e293b",
  margin: "15px auto",
  padding: "20px",
  borderRadius: "18px",
  width: "90%",
  maxWidth: "420px",
  textAlign: "center",
};

const tituloStyle: CSSProperties = {
  fontSize: "22px",
  marginBottom: "15px",
};

const linhaStyle: CSSProperties = {
  display: "flex",
  justifyContent: "center",
  alignItems: "center",
  textAlign: "center",
  gap: "10px",
  fontSize: "22px",
  margin: "15px",
};

const botaoBaseStyle: CSSProperties = {
  fontSize: "18px",
  padding: "12px 15px",
  margin: "5px",
  borderRadius: "12px",
  border: 0,
  color: "white",
  cursor: "pointer",
};

export default function Home() {
  const [ultima, setUltima] = useState<Leitura | null>(null);
  const [historico, setHistorico] = useState<Leitura[]>([]);
  const [aEnviar, setAEnviar] = useState(false);

  const carregar = useCallback(async () => {
    try {
      const res = await fetch("/api/leituras", { cache: "no-store" });
      const dados = await res.json();
      setUltima(dados.ultima);
      setHistorico(dados.historico);
    } catch (e) {
      console.error(e);
    }
  }, []);

  useEffect(() => {
    carregar();
    const t = setInterval(carregar, 5000);
    return () => clearInterval(t);
  }, [carregar]);

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
    return (
      <main
        style={{
          minHeight: "100vh",
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          textAlign: "center",
        }}
      >
        A carregar dados...
      </main>
    );
  }

  return (
    <main style={{ minHeight: "100vh", textAlign: "center", paddingBottom: "40px" }}>
      <h1
        style={{
          background: "#0f172a",
          padding: "20px",
          margin: 0,
          color: "#ff9800",
          fontSize: "36px",
          fontWeight: "bold",
        }}
      >
        🔥 AQ-CONTROL
      </h1>

      <div style={cardStyle}>
        <div style={tituloStyle}>🕒 Data e Hora</div>
        <div style={{ fontSize: "38px", fontWeight: "bold", color: "#00e676" }}>
          {new Date(ultima.criadoEm).toLocaleTimeString("pt-PT", {
            hour: "2-digit",
            minute: "2-digit",
          })}
        </div>
        <div style={{ fontSize: "20px", color: "#cbd5e1" }}>
          {new Date(ultima.criadoEm).toLocaleDateString("pt-PT")}
        </div>
      </div>

      <div style={cardStyle}>
        <div style={tituloStyle}>⚙️ Modo Caldeira</div>
        <div
          style={{
            display: "flex",
            justifyContent: "center",
            alignItems: "center",
            flexWrap: "nowrap",
            gap: "5px",
          }}
        >
          {MODOS.map((m) => (
            <button
              key={m}
              onClick={() => enviarComando("modo", m)}
              style={{
                fontSize: "18px",
                padding: "10px 14px",
                margin: "3px",
                borderRadius: "12px",
                border: 0,
                color: "white",
                cursor: "pointer",
                whiteSpace: "nowrap",
                background: ultima.modo === m ? "#ff9800" : "#334155",
                boxShadow: ultima.modo === m ? "0 0 12px #ff9800" : "none",
              }}
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

      <div style={cardStyle}>
        <div style={tituloStyle}>🌡 Temperaturas</div>
        <div style={linhaStyle}>
          <span>🔥 Caldeira:</span>
          <span>{ultima.tempCaldeira.toFixed(1)} °C</span>
        </div>
        <div style={linhaStyle}>
          <span>🚿 AQS:</span>
          <span>{ultima.tempAQS.toFixed(1)} °C</span>
        </div>
      </div>

      <div style={cardStyle}>
        <div style={tituloStyle}>⚡ Saídas</div>
        <Linha label="Ordem Caldeira:" ligado={ultima.rele1Ligado} />
        <Linha label="Bomba Caldeira:" ligado={ultima.rele2Ligado} />
        <Linha label="Bomba Aquecimento:" ligado={ultima.rele3Ligado} />

        <button
          onClick={() =>
            enviarComando("radiadores_pausa", ultima.radiadoresPausados ? "0" : "1")
          }
          style={{
            ...botaoBaseStyle,
            width: "90%",
            marginTop: "15px",
            background: ultima.radiadoresPausados ? "#ff9800" : "#334155",
            boxShadow: ultima.radiadoresPausados ? "0 0 12px #ff9800" : "none",
          }}
        >
          {ultima.radiadoresPausados ? "▶️ Retomar Radiadores" : "⏸️ Pausar Radiadores"}
        </button>
      </div>

      <div style={cardStyle}>
        <div style={tituloStyle}>📈 Histórico de Temperaturas (24h)</div>
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
    </main>
  );
}

function Linha({ label, ligado }: { label: string; ligado: boolean }) {
  return (
    <div style={linhaStyle}>
      <span>{label}</span>
      <span style={{ color: ligado ? "#00ff00" : "#ff3333", fontWeight: "bold" }}>
        {ligado ? "🟢 ON" : "🔴 OFF"}
      </span>
    </div>
  );
}
