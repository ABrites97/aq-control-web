# AQ-CONTROL Web

Dashboard remoto para o AQ-CONTROL (ESP32) — histórico de temperaturas,
gráficos, e controlo remoto do modo/radiadores.

## Como o ESP32 fala com isto

O ESP32 está atrás do router de casa, por isso é sempre ele a "ligar para
fora", nunca o contrário:

- `POST /api/leituras` — o ESP32 envia o estado atual periodicamente.
- `GET /api/comandos` — o ESP32 pergunta se há comandos por aplicar
  (criados pelo dashboard quando carregas num botão).
- `PATCH /api/comandos/:id` — o ESP32 marca um comando como aplicado.

Estas três rotas exigem um header `x-api-key` com a mesma chave que
configurares em `ESP32_API_KEY`. As rotas usadas só pelo dashboard
(`POST /api/comandos`, `GET /api/leituras`) não exigem essa chave.

## Passos para pôr a correr

### 1. Instalar dependências

```bash
npm install
```

### 2. Criar a base de dados na Neon

1. Cria uma conta grátis em https://neon.tech
2. Cria um novo projeto (ex: "aqcontrol")
3. No dashboard do projeto, vai a "Connect" e copia as duas connection
   strings: a "pooled" (para `DATABASE_URL`) e a "direct"/"unpooled"
   (para `DIRECT_URL`)

### 3. Configurar as variáveis de ambiente

```bash
cp .env.example .env
```

Edita o `.env` e cola as connection strings da Neon, e inventa uma
chave longa e aleatória para `ESP32_API_KEY` (vais usar a mesma no
firmware do ESP32).

### 4. Criar as tabelas na base de dados

```bash
npx prisma db push
```

### 5. Correr localmente

```bash
npm run dev
```

Abre http://localhost:3000 — deve aparecer "A carregar dados..." até
teres o ESP32 a enviar a primeira leitura.

### 6. Publicar (Vercel)

1. Cria um repositório no GitHub e envia este código para lá
   (`git init`, `git add .`, `git commit`, `git push`)
2. Cria uma conta grátis em https://vercel.com e liga-a ao GitHub
3. Importa o repositório na Vercel
4. Em "Environment Variables", adiciona `DATABASE_URL`, `DIRECT_URL`
   e `ESP32_API_KEY` com os mesmos valores do teu `.env`
5. Deploy — a Vercel dá-te um URL público (ex: `aqcontrol.vercel.app`)

## Nota sobre o plano grátis da Neon

A base de dados "adormece" ao fim de alguns minutos sem atividade. O
primeiro pedido a seguir a isso demora um pouco mais (1-2 segundos) —
é normal, não é um erro.
