# Contrato: Shell integration

**Since:** 0.12.0 (planned, gate Etapa 6)
**Status:** planned
**Fatia:** `medium-term/v0.12-shell-integration-gate-etapa6.md`

## Tipos aditivos

```c
CAPY_WIDGET_NOTIFICATION
CAPY_WIDGET_TRAY_ICON
```

## Notification widget

- `widget->text` = título da notificação.
- `widget->user_data` = payload customizado.
- Style ganha:
  - `uint16_t timeout_ms` (0 = persistente).
  - `uint8_t urgency` (0=low, 1=normal, 2=critical).

## Tray icon widget

- `widget->text` = tooltip.
- Renderiza como ícone no system tray.
- Click dispara `on_click`.

## Responsabilidades do host

- Renderizar notification em região reservada (não no client area do app).
- Empilhar múltiplas notifications.
- Honrar timeout (auto-dismiss).
- Tray icon visível em barra do sistema.
- Política de foco entre janelas (alt-tab, win+tab).

## Invariantes CapyUI

- Tipos sem shell real: existem no modelo, não renderizam (sem-op em display-list).
- Hosts antigos (< Etapa 6) ignoram esses tipos sem crash.
- `dispatch_event` em widget desses tipos é no-op se host não conecta.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.12.0 (planned) | Introdução de tipos NOTIFICATION/TRAY_ICON |
