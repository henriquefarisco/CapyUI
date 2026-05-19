# Contrato: Adapter CapyOS

**Since:** 0.7.0 (planned, gate Etapa 4)
**Status:** planned
**Fatia:** `medium-term/v0.7-host-adapter-gate-etapa4.md`

## Escopo

**Único contrato CapyUI que cita estruturas CapyOS.** O adapter vive no lado CapyOS (`CapyOS/src/ui_adapter/`).

## Responsabilidades do adapter

1. **Mapeamento de eventos:**
   - `gui_event` (CapyOS) → `capy_widget_event`.
   - Pointer/key/wheel/touch como aplicável.
   - Modifiers (CTRL/ALT/SHIFT/META) propagados.
2. **Tema do SO → `capy_theme`:**
   - Detectar variante (light/dark) do SO.
   - Aplicar via `capy_theme_apply`.
   - Re-aplicar quando usuário troca em settings.
3. **Locale do SO → `capy_locale` (since 0.13):**
   - Tag BCP 47.
   - RTL flag.
   - Plural rule.
4. **Roteamento por frame:**
   ```
   loop:
     events ← collect_input_from_capyos
     for ev in events:
       capy_widget_dispatch_key/pointer(root, ev)
     now ← monotonic_ms_capyos
     capy_widget_tick(root, now)
     capy_widget_measure(root, surface.width, surface.height)
     capy_widget_arrange(root)
     capy_widget_emit(root, &dl)
     compositor_render(surface, dl)
   ```
5. **Lifecycle:** init, frame loop, shutdown sem leaks.

## Restrições inegociáveis

- **CapyUI não inclui headers CapyOS** (`<gui/*>`, `<kernel/*>`, etc.).
- **Adapter não modifica state CapyUI fora das APIs públicas** (read-only do modelo retido após emit).
- **Determinismo preservado:** adapter pode acrescentar não-determinismo (clock real, input), mas modelo CapyUI continua determinístico dado o input.

## Validação

Cross-rep audit:

```sh
# CapyUI deve estar limpo de includes CapyOS
grep -rn '#include' /Volumes/CapyOS/CapyUI/src/widget/ | grep -E 'gui|kernel|capyos'
```

Esperado: vazio.

## Documento espelhado

`CapyOS/docs/reference/integration/capyui-widget-integration-contract.md` (lado CapyOS).

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.7.0 (planned) | Introdução do adapter (gate Etapa 4) |
