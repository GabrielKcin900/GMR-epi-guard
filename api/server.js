/*
 * EPI Guard API — POST /check (plano §6).
 * Escute em 0.0.0.0 para o ESP32 alcançar pela rede WiFi.
 */

const express = require("express");
const db = require("./db.json");

const app = express();
const PORT = process.env.PORT || 3000;

app.use(express.json());

function epiInList(name, items) {
  const norm = (s) =>
    s
      .normalize("NFD")
      .replace(/[\u0300-\u036f]/g, "")
      .toLowerCase();

  const target = norm(name);
  return items.some((item) => norm(item) === target);
}

app.post("/check", (req, res) => {
  const { who, with: items = [] } = req.body;

  if (!who || typeof who !== "string") {
    return res.status(400).json({ error: "campo 'who' obrigatorio" });
  }

  const required = db.people[who];

  if (!required) {
    return res.json({
      allowed: false,
      unknown: true,
      required: [],
      missing: [],
    });
  }

  const missing = required.filter((r) => !epiInList(r, items));

  res.json({
    allowed: missing.length === 0,
    unknown: false,
    required,
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
