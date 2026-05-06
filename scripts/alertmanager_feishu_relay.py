#!/usr/bin/env python3
import argparse
import json
import os
import urllib.error
import urllib.request
from datetime import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer


def _severity_rank(severity: str) -> int:
    order = {"critical": 3, "warning": 2, "info": 1}
    return order.get(severity.lower(), 0)


def _summarize_alerts(payload: dict | list) -> str:
    if isinstance(payload, dict):
        alerts = payload.get("alerts", []) if isinstance(payload.get("alerts", []), list) else []
        top_status = payload.get("status", "unknown")
        top_receiver = payload.get("receiver", "unknown")
    else:
        alerts = payload
        top_status = "unknown"
        top_receiver = "unknown"

    if not alerts:
        return "Alertmanager 通知\n\n暂无告警数据"

    def _alert_key(alert: dict) -> tuple[int, str]:
        labels = alert.get("labels", {}) if isinstance(alert, dict) else {}
        severity = labels.get("severity", "")
        name = labels.get("alertname", "")
        return (-_severity_rank(severity), name)

    sorted_alerts = sorted(alerts, key=_alert_key)
    max_severity = "unknown"
    for alert in sorted_alerts:
        labels = alert.get("labels", {}) if isinstance(alert, dict) else {}
        severity = labels.get("severity", "unknown")
        if _severity_rank(severity) > _severity_rank(max_severity):
            max_severity = severity

    first = sorted_alerts[0]
    receiver = first.get("receiver", top_receiver) if isinstance(first, dict) else top_receiver

    lines = [
        "Alertmanager 通知",
        f"时间: {datetime.utcnow().isoformat()}Z",
        f"级别: {max_severity}",
        f"接收器: {receiver}",
        f"状态: {top_status}",
        f"告警数: {len(sorted_alerts)}",
        "",
    ]

    for index, alert in enumerate(sorted_alerts, start=1):
        labels = alert.get("labels", {}) if isinstance(alert, dict) else {}
        annotations = alert.get("annotations", {}) if isinstance(alert, dict) else {}
        alert_status = alert.get("status", "unknown") if isinstance(alert, dict) else "unknown"
        lines.append(
            f"{index}. [{labels.get('severity', 'unknown')}] {labels.get('alertname', 'unknown')} "
            f"({alert_status})"
        )
        summary = annotations.get("summary")
        description = annotations.get("description")
        starts_at = alert.get("startsAt", "")
        ends_at = alert.get("endsAt", "")
        generator_url = alert.get("generatorURL", "")
        if summary:
            lines.append(f"   summary: {summary}")
        if description:
            lines.append(f"   description: {description}")
        if starts_at:
            lines.append(f"   startsAt: {starts_at}")
        if ends_at:
            lines.append(f"   endsAt: {ends_at}")
        if generator_url:
            lines.append(f"   generatorURL: {generator_url}")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def _post_to_feishu(webhook_url: str, message_text: str) -> tuple[int, str]:
    payload = {
        "msg_type": "text",
        "content": {"text": message_text},
    }
    request = urllib.request.Request(
        webhook_url,
        data=json.dumps(payload, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    with urllib.request.urlopen(request, timeout=15) as response:
        response_text = response.read().decode("utf-8", errors="replace")
        return response.status, response_text


class RelayHandler(BaseHTTPRequestHandler):
    server_version = "AlertmanagerFeishuRelay/1.0"

    def do_GET(self):
        if self.path == "/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
            return

        self.send_response(404)
        self.end_headers()

    def do_POST(self):
        webhook_url = self.server.webhook_url  # type: ignore[attr-defined]
        length = int(self.headers.get("Content-Length", "0"))
        raw_body = self.rfile.read(length).decode("utf-8", errors="replace")

        try:
            payload = json.loads(raw_body)
        except json.JSONDecodeError as exc:
            self._fail(400, f"invalid json: {exc}")
            return

        if isinstance(payload, list):
            alerts = payload
        elif isinstance(payload, dict) and isinstance(payload.get("alerts"), list):
            alerts = payload["alerts"]
        else:
            alerts = []

        message_text = _summarize_alerts(alerts)
        now = datetime.utcnow().isoformat() + "Z"
        print(f"[{now}] path={self.path} alerts={len(alerts)}", flush=True)
        print(message_text, end="", flush=True)

        if not webhook_url:
            self._fail(500, "FEISHU_WEBHOOK_URL is not configured")
            return

        try:
            status, response_text = _post_to_feishu(webhook_url, message_text)
        except urllib.error.HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace") if exc.fp else str(exc)
            self._fail(502, f"feishu http error: {exc.code} {body}")
            return
        except Exception as exc:
            self._fail(502, f"feishu request failed: {exc}")
            return

        try:
            response_data = json.loads(response_text)
        except json.JSONDecodeError:
            response_data = {"raw": response_text}

        code = response_data.get("code", 0) if isinstance(response_data, dict) else 0
        msg = response_data.get("msg", "") if isinstance(response_data, dict) else ""
        if code not in (0, "0"):
            self._fail(502, f"feishu rejected payload: code={code} msg={msg} body={response_text}")
            return

        print(f"  feishu_status={status} code={code} msg={msg}", flush=True)
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"ok":true}')

    def log_message(self, format, *args):
        return

    def _fail(self, status: int, message: str):
        now = datetime.utcnow().isoformat() + "Z"
        print(f"[{now}] error={message}", flush=True)
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps({"ok": False, "error": message}).encode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description="Relay Alertmanager webhooks to Feishu custom bots")
    parser.add_argument("--port", type=int, default=int(os.environ.get("PORT", "8088")))
    args = parser.parse_args()

    webhook_url = os.environ.get("FEISHU_WEBHOOK_URL", "").strip()
    server = HTTPServer(("0.0.0.0", args.port), RelayHandler)
    server.webhook_url = webhook_url  # type: ignore[attr-defined]
    print(f"alertmanager-feishu-relay listening on :{args.port}", flush=True)
    print(f"feishu_webhook_configured={'yes' if webhook_url else 'no'}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())