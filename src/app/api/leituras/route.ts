import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

// Usado pelo DASHBOARD (browser) para mostrar os dados - sem chave, é público
export async function GET() {
  const ultima = await prisma.leitura.findFirst({
    orderBy: { criadoEm: "desc" },
  });

  const historicoDesc = await prisma.leitura.findMany({
    orderBy: { criadoEm: "desc" },
    take: 50,
  });

  return NextResponse.json({
    ultima,
    historico: historicoDesc.reverse(), // do mais antigo para o mais recente, para o gráfico
  });
}

// Usado pelo ESP32 para enviar uma nova leitura - exige a chave secreta
export async function POST(req: NextRequest) {
  const apiKey = req.headers.get("x-api-key");

  if (apiKey !== process.env.ESP32_API_KEY) {
    return NextResponse.json({ erro: "nao autorizado" }, { status: 401 });
  }

  const body = await req.json();

  const leitura = await prisma.leitura.create({
    data: {
      tempCaldeira: body.tempCaldeira,
      tempAQS: body.tempAQS,
      rele1Ligado: body.rele1Ligado,
      rele2Ligado: body.rele2Ligado,
      rele3Ligado: body.rele3Ligado,
      modo: body.modo,
      radiadoresPausados: body.radiadoresPausados,
    },
  });

  return NextResponse.json({ ok: true, id: leitura.id });
}
