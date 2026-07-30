"use client";

import { useEffect, useState } from "react";
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

async function enviarComando(tipo: string, valor: string) {
  await fetch("/api/comandos", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ tipo, valor }),
  });
}

export default function Dashboard() {
  const [leituras, setLeituras] = useState<Leitura[]>([]);
  const [aEnviar, setAEnviar] = useState(false);

  async function atualizar() {
    const res = await fetch("/api/leituras?limite=200");
    const dados = await res.json();
    setLeituras(dados);
  }

  useEffect(() => {
    atualizar();
    const intervalo = setInterval(atualizar, 30000); // atualiza a cada 30s
    return () => clearInterval(intervalo);
  }, []);

  const ultima = leituras[leituras.length - 1];

  const dadosGrafico = leituras.map((l) => ({
    hora: new Date(l.criadoEm).toLocaleTimeString("pt-PT", {
      hour: "2-digit",
      minute: "2-digit",
    }),
    Caldeira: l.tempCaldeira,
    AQS: l.tempAQS,
  }));

  async function mudarModo(modo: string) {
    setAEnviar(true);
    await enviarComando("modo", modo);
    setAEnviar(false);
  }

  async function alternarPausaRadiadores() {
    setAEnviar(true);
    const novoValor = ultima?.radiadoresPausados ? "0" : "1";
    await enviarComando("radiadores_pausa", novoValor);
    setAEnviar(false);
  }

  return (
    <main className="max-w-3xl mx-auto p-6 space-y-6">
      <h1 className="text-3xl font-bold text-orange-500">🔥 AQ-CONTROL</h1>

      {!ultima && <p className="text-slate-400">A carregar dados...</p>}

      {ultima && (
        <>
          <section className="bg-slate-800 rounded-2xl p-5 grid grid-cols-2 gap-4">
            <div>
              <p className="text-slate-400 text-sm">Caldeira</p>
              <p className="text-3xl font-bold text-green-400">
                {ultima.tempCaldeira.toFixed(1)}°C
              </p>
            </div>
            <div>
              <p className="text-slate-400 text-sm">AQS</p>
              <p className="text-3xl font-bold text-green-400">
                {ultima.tempAQS.toFixed(1)}°C
              </p>
            </div>
            <div>
              <p className="text-slate-400 text-sm">Modo atual</p>
              <p className="text-xl font-semibold">{ultima.modo}</p>
            </div>
            <div>
              <p className="text-slate-400 text-sm">Última leitura</p>
              <p className="text-xl">
                {new Date(ultima.criadoEm).toLocaleString("pt-PT")}
              </p>
            </div>
          </section>

          <section className="bg-slate-800 rounded-2xl p-5">
            <h2 className="text-lg font-semibold mb-3">Saídas</h2>
            <div className="flex gap-6">
              <span className={ultima.rele1Ligado ? "text-green-400" : "text-red-400"}>
                Ordem Caldeira: {ultima.rele1Ligado ? "ON" : "OFF"}
              </span>
              <span className={ultima.rele2Ligado ? "text-green-400" : "text-red-400"}>
                Bomba Caldeira: {ultima.rele2Ligado ? "ON" : "OFF"}
              </span>
              <span className={ultima.rele3Ligado ? "text-green-400" : "text-red-400"}>
                Bomba Aquecimento: {ultima.rele3Ligado ? "ON" : "OFF"}
              </span>
            </div>
          </section>

          <section className="bg-slate-800 rounded-2xl p-5">
            <h2 className="text-lg font-semibold mb-3">Modo Caldeira</h2>
            <div className="flex gap-2 flex-wrap">
              {["ON", "INVERNO", "VERAO", "OFF"].map((modo) => (
                <button
                  key={modo}
                  disabled={aEnviar}
                  onClick={() => mudarModo(modo)}
                  className={
                    "px-4 py-2 rounded-xl font-medium " +
                    (ultima.modo === modo
                      ? "bg-orange-500 text-white"
                      : "bg-slate-700 hover:bg-slate-600")
                  }
                >
                  {modo}
                </button>
              ))}
            </div>

            <button
              disabled={aEnviar}
              onClick={alternarPausaRadiadores}
              className={
                "mt-4 w-full px-4 py-2 rounded-xl font-medium " +
                (ultima.radiadoresPausados
                  ? "bg-orange-500 text-white"
                  : "bg-slate-700 hover:bg-slate-600")
              }
            >
              {ultima.radiadoresPausados
                ? "▶️ Retomar Radiadores"
                : "⏸️ Pausar Radiadores"}
            </button>
          </section>

          <section className="bg-slate-800 rounded-2xl p-5">
            <h2 className="text-lg font-semibold mb-3">
              Histórico de Temperaturas
            </h2>
            <ResponsiveContainer width="100%" height={300}>
              <LineChart data={dadosGrafico}>
                <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                <XAxis dataKey="hora" stroke="#94a3b8" />
                <YAxis stroke="#94a3b8" />
                <Tooltip
                  contentStyle={{ backgroundColor: "#1e293b", border: "none" }}
                />
                <Legend />
                <Line type="monotone" dataKey="Caldeira" stroke="#f97316" dot={false} />
                <Line type="monotone" dataKey="AQS" stroke="#22c55e" dot={false} />
              </LineChart>
            </ResponsiveContainer>
          </section>
        </>
      )}
    </main>
  );
}
