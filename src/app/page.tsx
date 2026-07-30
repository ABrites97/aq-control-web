"use client";

import { useEffect, useState, useCallback, ReactNode } from "react";
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
      <main className="min-h-screen flex items-center justify-center text-center">
        A carregar dados...
      </main>
    );
  }

  return (
    <main className="min-h-screen text-center pb-16">
      <h1 className="bg-[#0f172a] text-[#ff9800] text-3xl py-5 m-0">
        🔥 AQ-CONTROL
      </h1>

      <Card titulo="🕒 Data e Hora">
        <div className="text-3xl font-bold text-[#00e676]">
          {new Date(ultima.criadoEm).toLocaleTimeString("pt-PT", {
            hour: "2-digit",
            minute: "2-digit",
          })}
        </div>
        <div className="text-lg text-[#cbd5e1] mt-1">
          {new Date(ultima.criadoEm).toLocaleDateString("pt-PT")}
        </div>
      </Card>

      <Card titulo="⚙️ Modo Caldeira">
        <div className="flex justify-center flex-nowrap gap-1">
          {MODOS.map((m) => (
            <button
              key={m}
              onClick={() => enviarComando("modo", m)}
              className={`text-sm sm:text-lg px-2 sm:px-4 py-2 rounded-xl font-semibold transition whitespace-nowrap ${
                ultima.modo === m
                  ? "bg-[#ff9800] shadow-[0_0_12px_#ff9800]"
                  : "bg-[#334155] hover:bg-[#ff9800]"
              }`}
            >
              {m === "VERAO" ? "VERÃO" : m}
            </button>
          ))}
        </div>
        {aEnviar && (
          <div className="text-sm text-[#ff9800] mt-3">A enviar comando...</div>
        )}
      </Card>

      <Card titulo="🌡 Temperaturas">
        <Linha label="🔥 Caldeira:" valor={`${ultima.tempCaldeira.toFixed(1)} °C`} />
        <Linha label="🚿 AQS:" valor={`${ultima.tempAQS.toFixed(1)} °C`} />
      </Card>

      <Card titulo="⚡ Saídas">
        <Linha
          label="Ordem Caldeira:"
          valor={ultima.rele1Ligado ? "🟢 ON" : "🔴 OFF"}
          ligado={ultima.rele1Ligado}
        />
        <Linha
          label="Bomba Caldeira:"
          valor={ultima.rele2Ligado ? "🟢 ON" : "🔴 OFF"}
          ligado={ultima.rele2Ligado}
        />
        <Linha
          label="Bomba Aquecimento:"
          valor={ultima.rele3Ligado ? "🟢 ON" : "🔴 OFF"}
          ligado={ultima.rele3Ligado}
        />

        <button
          onClick={() =>
            enviarComando("radiadores_pausa", ultima.radiadoresPausados ? "0" : "1")
          }
          className={`text-lg px-5 py-3 mt-4 rounded-xl font-semibold transition w-full ${
            ultima.radiadoresPausados
              ? "bg-[#ff9800] shadow-[0_0_12px_#ff9800]"
              : "bg-[#334155] hover:bg-[#ff9800]"
          }`}
        >
          {ultima.radiadoresPausados ? "▶️ Retomar Radiadores" : "⏸️ Pausar Radiadores"}
        </button>
      </Card>

      <Card titulo="📈 Histórico de Temperaturas (24h)">
        <div className="h-64 w-full">
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
      </Card>
    </main>
  );
}

function Card({ titulo, children }: { titulo: string; children: ReactNode }) {
  return (
    <div className="bg-[#1e293b] mx-auto my-4 p-5 rounded-2xl w-[90%] max-w-md">
      <div className="text-xl mb-3">{titulo}</div>
      {children}
    </div>
  );
}

function Linha({
  label,
  valor,
  ligado,
}: {
  label: string;
  valor: string;
  ligado?: boolean;
}) {
  const cor =
    ligado === undefined ? "" : ligado ? "text-[#00ff00] font-bold" : "text-[#ff3333] font-bold";

  return (
    <div className="flex justify-center items-center gap-2 text-lg my-2">
      <span>{label}</span>
      <span className={cor}>{valor}</span>
    </div>
  );
}
