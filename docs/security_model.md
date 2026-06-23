# Security Model

EdgeGuard is designed for same-network local use. It does not depend on cloud services and does not commit real credentials.

- Wi-Fi station credentials, fallback AP credentials, and the local control token are configured in ignored `secrets.h`.
- GET routes are local read routes.
- POST control routes require `X-EdgeGuard-Token` when `EDGEGUARD_API_TOKEN` is non-empty.
- The token is never returned by a GET route and is not printed to Serial.
- Dashboard controls are same-origin; wildcard CORS is not used for control routes.
- Relay outputs are documented for low-voltage LED loads only.
