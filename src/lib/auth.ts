import { NextRequest } from "next/server";

// Confirma que o pedido traz a chave secreta correta no header "x-api-key".
// Usado para proteger as rotas que o ESP32 chama (nao qualquer pessoa deve poder
// escrever leituras falsas ou mandar comandos).
export function chaveApiValida(req: NextRequest): boolean {
  const chaveRecebida = req.headers.get("x-api-key");
  const chaveEsperada = process.env.ESP32_API_KEY;

  if (!chaveEsperada) {
    // Sem chave configurada no servidor, nunca deixa passar (falha em seguranca)
    return false;
  }

  return chaveRecebida === chaveEsperada;
}
