import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { chaveApiValida } from "@/lib/auth";

// O ESP32 chama isto periodicamente (ex: a cada 5 minutos) para gravar o estado atual
export async function POST(req: NextRequest) {
  if (!chaveApiValida(req)) {
    return NextResponse.json({ erro: "Chave de API invalida" }, { status: 401 });
  }

  const dados = await req.json();

  const camposEmFalta = [
    "tempCaldeira",
    "tempAQS",
    "rele1Ligado",
    "rele2Ligado",
    "rele3Ligado",
    "modo",
    "radiadoresPausados",
  ].filter((campo) => dados[campo] === undefined);

  if (camposEmFalta.length > 0) {
    return NextResponse.json(
      { erro: "Campos em falta: " + camposEmFalta.join(", ") },
      { status: 400 }
    );
  }

  const leitura = await prisma.leitura.create({
    data: {
      tempCaldeira: dados.tempCaldeira,
      tempAQS: dados.tempAQS,
      rele1Ligado: dados.rele1Ligado,
      rele2Ligado: dados.rele2Ligado,
      rele3Ligado: dados.rele3Ligado,
      modo: dados.modo,
      radiadoresPausados: dados.radiadoresPausados,
    },
  });

  return NextResponse.json({ ok: true, id: leitura.id });
}

// O dashboard chama isto para desenhar o gráfico - devolve as últimas N leituras
export async function GET(req: NextRequest) {
  const { searchParams } = new URL(req.url);
  const limite = Math.min(Number(searchParams.get("limite")) || 200, 1000);

  const leituras = await prisma.leitura.findMany({
    orderBy: { criadoEm: "desc" },
    take: limite,
  });

  return NextResponse.json(leituras.reverse());
}
