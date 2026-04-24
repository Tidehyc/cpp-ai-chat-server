## GitHub Copilot Chat

- Extension: 0.43.0 (prod)
- VS Code: 1.115.0 (41dd792b5e652393e7787322889ed5fdc58bd75b)
- OS: linux 6.17.0-19-generic x64
- GitHub Account: Tidehyc

## Network

User Settings:
```json
  "http.systemCertificatesNode": true,
  "github.copilot.advanced.debug.useElectronFetcher": true,
  "github.copilot.advanced.debug.useNodeFetcher": false,
  "github.copilot.advanced.debug.useNodeFetchFetcher": true
```

Connecting to https://api.github.com:
- DNS ipv4 Lookup: 20.205.243.168 (15 ms)
- DNS ipv6 Lookup: Error (122 ms): getaddrinfo ENOTFOUND api.github.com
- Proxy URL: http://127.0.0.1:7890 (1 ms)
- Proxy Connection: 200 Connection established (4 ms)
- Electron fetch (configured): timed out after 10 seconds
- Node.js https: Error (5011 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)
- Node.js fetch: Error (5043 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///snap/code/233/usr/share/code/resources/app/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: read ECONNRESET
  	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)

Connecting to https://api.githubcopilot.com/_ping:
- DNS ipv4 Lookup: 140.82.114.21 (10 ms)
- DNS ipv6 Lookup: Error (14 ms): getaddrinfo ENOTFOUND api.githubcopilot.com
- Proxy URL: http://127.0.0.1:7890 (2 ms)
- Proxy Connection: 200 Connection established (6 ms)
- Electron fetch (configured): Error (5120 ms): Error: net::ERR_CONNECTION_RESET
	at SimpleURLLoaderWrapper.<anonymous> (node:electron/js2c/utility_init:2:10684)
	at SimpleURLLoaderWrapper.emit (node:events:519:28)
  {"is_request_error":true,"network_process_crashed":false}
- Node.js https: Error (171 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)
- Node.js fetch: Error (239 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///snap/code/233/usr/share/code/resources/app/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: read ECONNRESET
  	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)

Connecting to https://copilot-proxy.githubusercontent.com/_ping:
- DNS ipv4 Lookup: 4.249.131.160 (33 ms)
- DNS ipv6 Lookup: Error (72 ms): getaddrinfo ENOTFOUND copilot-proxy.githubusercontent.com
- Proxy URL: http://127.0.0.1:7890 (71 ms)
- Proxy Connection: 200 Connection established (9 ms)
- Electron fetch (configured): Error (259 ms): Error: net::ERR_CONNECTION_RESET
	at SimpleURLLoaderWrapper.<anonymous> (node:electron/js2c/utility_init:2:10684)
	at SimpleURLLoaderWrapper.emit (node:events:519:28)
  {"is_request_error":true,"network_process_crashed":false}
- Node.js https: Error (164 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)
- Node.js fetch: Error (136 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/tide/.vscode/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///snap/code/233/usr/share/code/resources/app/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: read ECONNRESET
  	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)

Connecting to https://mobile.events.data.microsoft.com: HTTP 404 (602 ms)
Connecting to https://dc.services.visualstudio.com: Error (351 ms): Error: net::ERR_CONNECTION_RESET
	at SimpleURLLoaderWrapper.<anonymous> (node:electron/js2c/utility_init:2:10684)
	at SimpleURLLoaderWrapper.emit (node:events:519:28)
  {"is_request_error":true,"network_process_crashed":false}
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (111 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (222 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)
Connecting to https://default.exp-tas.com: Error (127 ms): Error: read ECONNRESET
	at TLSWrap.onStreamRead (node:internal/stream_base_commons:216:20)

Number of system certificates: 432

## Documentation

In corporate networks: [Troubleshooting firewall settings for GitHub Copilot](https://docs.github.com/en/copilot/troubleshooting-github-copilot/troubleshooting-firewall-settings-for-github-copilot).