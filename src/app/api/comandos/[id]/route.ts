import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";

// Usado pelo ESP32 para marcar um comando como aplicado - exige a chave secreta
export async function PATCH(
  req: NextRequest,
  { params }: { params: { id: string } }
) {
  const apiKey = req.headers.get("x-api-key");

  if (apiKey !== process.env.ESP32_API_KEY) {
    return NextResponse.json({ erro: "nao autorizado" }, { status: 401 });
  }

  const id = parseInt(params.id, 10);

  const comando = await prisma.comando.update({
    where: { id },
    data: { aplicado: true, aplicadoEm: new Date() },
  });

  return NextResponse.json({ ok: true, comando });
}
