/*
 * EPI Guard API — POST /check (plano §6).
 * Escute em 0.0.0.0 para o ESP32 alcançar pela rede WiFi.
 */

const express = require("express");
const db = require("./db.json");

const app = express();
const PORT = process.env.PORT || 3000;

app.use(express.json());

function logLine(msg) {
  const ts = new Date().toLocaleTimeString("pt-BR", { hour12: false });
  console.log(`[${ts}] ${msg}`);
}

app.use((req, _res, next) => {
  logLine(`${req.method} ${req.path} <- ${req.ip || req.socket.remoteAddress}`);
  next();
});

function toAscii(s) {
  return s
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "");
}

function epiInList(name, items) {
  const target = toAscii(name).toLowerCase();
  return items.some((item) => toAscii(item).toLowerCase() === target);
}

app.post("/check", (req, res) => {
  const { who, with: items = [] } = req.body;

  if (!who || typeof who !== "string") {
    logLine("POST /check -> 400 (campo 'who' obrigatorio)");
    return res.status(400).json({ error: "campo 'who' obrigatorio" });
  }

  const required = db.people[who];

  if (!required) {
    logLine(`POST /check who=${who} -> NAO CADASTRADO`);
    return res.json({
      allowed: false,
      unknown: true,
      required: [],
      missing: [],
    });
  }

  const requiredAscii = required.map(toAscii);
  const missing = requiredAscii.filter((r) => !epiInList(r, items));
  const allowed = missing.length === 0;

  logLine(
    `POST /check who=${who} with=[${items.join(", ")}] -> ` +
      (allowed ? "LIBERADO" : `NEGADO (falta: ${missing.join(", ")})`)
  );

  res.json({
    allowed,
    unknown: false,
    required: requiredAscii,
    missing,
  });
});

app.get("/health", (_req, res) => {
  res.json({ status: "ok" });
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`EPI Guard API em http://0.0.0.0:${PORT}`);
  console.log("POST /check  { who, with[] }");
});
