import { PrismaClient } from "@prisma/client";

// Evita criar uma nova ligação à base de dados a cada pedido em desenvolvimento
// (o Next.js recarrega módulos frequentemente em dev mode)
const globalForPrisma = globalThis as unknown as {
  prisma: PrismaClient | undefined;
};

export const prisma = globalForPrisma.prisma ?? new PrismaClient();

if (process.env.NODE_ENV !== "production") {
  globalForPrisma.prisma = prisma;
}
