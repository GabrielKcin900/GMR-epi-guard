(function () {
  const form = document.getElementById("form");
  const btn = document.getElementById("btn");
  const resultEl = document.getElementById("result");

  function showResult(kind, html) {
    resultEl.className = "show " + kind;
    resultEl.innerHTML = html;
  }

  async function verificar(ev) {
    ev.preventDefault();

    const who = document.getElementById("who").value.trim();
    const items = [...document.querySelectorAll(".epi:checked")].map(function (c) {
      return c.value;
    });

    if (!who) {
      showResult("err", "Informe o nome.");
      return;
    }

    btn.disabled = true;
    showResult("err", "Verificando...");

    try {
      const r = await fetch("/verify", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ who: who, with: items }),
      });

      let data;
      try {
        data = await r.json();
      } catch (e) {
        showResult("err", "Resposta invalida do firmware (HTTP " + r.status + ").");
        return;
      }

      if (data.status === "error") {
        showResult("err", "Erro na verificacao (rede/API ou timeout).");
        return;
      }

      if (data.unknown_person) {
        showResult("deny", "<strong>NAO CADASTRADO</strong><br>Pessoa nao encontrada na base.");
        return;
      }

      if (data.allowed) {
        showResult("ok", "<strong>LIBERADO</strong><br>Todos os EPIs obrigatorios presentes.");
        return;
      }

      const missing = (data.missing || []).join(", ") || "(nenhum listado)";
      showResult("deny", "<strong>NEGADO</strong><br>Falta: " + missing);
    } catch (e) {
      showResult("err", "Falha de comunicacao com o firmware.");
    } finally {
      btn.disabled = false;
    }
  }

  form.addEventListener("submit", verificar);
})();
