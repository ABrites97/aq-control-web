import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { chaveApiValida } from "@/lib/auth";

// Chamado pelo dashboard (browser) quando carregas num botão - cria um comando
// novo que fica à espera de o ESP32 vir buscar. NAO exige chave de API porque
// é chamado pelo próprio site, do lado do servidor.
export async function POST(req: NextRequest) {
  const dados = await req.json();

  if (!dados.tipo || dados.valor === undefined) {
    return NextResponse.json({ erro: "tipo e valor sao obrigatorios" }, { status: 400 });
  }

  const comando = await prisma.comando.create({
    data: {
      tipo: dados.tipo,
      valor: String(dados.valor),
    },
  });

  return NextResponse.json({ ok: true, id: comando.id });
}

// Chamado pelo ESP32 periodicamente - devolve os comandos ainda não aplicados
export async function GET(req: NextRequest) {
  if (!chaveApiValida(req)) {
    return NextResponse.json({ erro: "Chave de API invalida" }, { status: 401 });
  }

  const pendentes = await prisma.comando.findMany({
    where: { aplicado: false },
    orderBy: { criadoEm: "asc" },
  });

  return NextResponse.json(pendentes);
}
