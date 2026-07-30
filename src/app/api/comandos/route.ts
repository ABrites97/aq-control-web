import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

// Usado pelo ESP32 para ir buscar comandos ainda não aplicados - exige a chave secreta
export async function GET(req: NextRequest) {
  const apiKey = req.headers.get("x-api-key");

  if (apiKey !== process.env.ESP32_API_KEY) {
    return NextResponse.json({ erro: "nao autorizado" }, { status: 401 });
  }

  const pendentes = await prisma.comando.findMany({
    where: { aplicado: false },
    orderBy: { criadoEm: "asc" },
  });

  return NextResponse.json(pendentes);
}

// Usado pelo DASHBOARD quando carregas num botão - sem chave, é público
export async function POST(req: NextRequest) {
  const body = await req.json();

  const comando = await prisma.comando.create({
    data: {
      tipo: body.tipo,
      valor: body.valor,
    },
  });

  return NextResponse.json({ ok: true, id: comando.id });
}
