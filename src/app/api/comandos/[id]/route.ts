import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { chaveApiValida } from "@/lib/auth";

// Chamado pelo ESP32 depois de aplicar um comando, para nao o voltar a aplicar
// na proxima vez que perguntar quais estao pendentes.
export async function PATCH(
  req: NextRequest,
  { params }: { params: { id: string } }
) {
  if (!chaveApiValida(req)) {
    return NextResponse.json({ erro: "Chave de API invalida" }, { status: 401 });
  }

  const id = Number(params.id);
  if (Number.isNaN(id)) {
    return NextResponse.json({ erro: "id invalido" }, { status: 400 });
  }

  await prisma.comando.update({
    where: { id },
    data: { aplicado: true, aplicadoEm: new Date() },
  });

  return NextResponse.json({ ok: true });
}
