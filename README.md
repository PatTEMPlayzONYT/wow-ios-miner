# WOW Lotto — iPhone RandomX miner (staged build)

A native iOS app to mine Wownero as a lottery ticket on an iPhone 13.
Built **without a Mac**: GitHub Actions compiles it on a free cloud macOS
runner; you install it with **SideStore**.

## Honest status

This is **Step 1 of 2**, on purpose:

- **Step 1 (this repo): a hashrate benchmark.** It proves RandomX actually
  runs on your iPhone and measures the *real* speed in the only config stock
  iOS allows — **interpreter + light mode, no JIT.** That number is the whole
  ballgame: it tells us whether a full miner is worth building. Expect it to
  be low (possibly single-to-low-double-digit H/s). JIT (via SideJITServer)
  and fast-mode could improve it — that's what we test next.
- **Step 2 (later, only if the number justifies it): the full solo miner** —
  block templates from the Wownero node, hashing, block signing, submit —
  reusing your existing wallet + spend key.

We build the cheap, decisive thing first instead of a huge miner that might
be capped at 20 H/s anyway.

## How the build works

```
push code ─▶ GitHub Actions (cloud Mac) ─▶ WowLotto-unsigned.ipa artifact
                                              │
                             download it ─────┘─▶ SideStore signs + installs
                                                   (your Apple ID, on-device)
```

## First-time setup

1. **Create a GitHub repo** and push these files:
   ```
   cd "C:\claude projects\wow-ios-miner"
   git init
   git add .
   git commit -m "Step 1: RandomX iOS benchmark"
   git branch -M main
   git remote add origin https://github.com/<you>/<repo>.git
   git push -u origin main
   ```
2. On GitHub, open the **Actions** tab → watch the `Build iOS app` run.
   - If it goes green, download the **WowLotto-unsigned-ipa** artifact.
   - If it fails, copy the failed step's log to Claude — first builds usually
     need a tweak or two (iOS cross-compile is finicky).
3. **Install SideStore** on the iPhone (one-time pairing needs this PC with
   iTunes + iCloud drivers). Then open the `.ipa` in SideStore to sign+install.
4. Launch **WOW Lotto**, tap **Run benchmark**, and tell Claude the H/s.

## Files
- `Sources/` — the app (SwiftUI) + `bench.c` (RandomX benchmark) + bridging header
- `project.yml` — xcodegen spec (generates the Xcode project in CI)
- `.github/workflows/ios-build.yml` — the cloud-Mac build
- RandomX itself is fetched fresh in CI from tevador/RandomX (not committed)

## Expectations (read before getting excited)
- **No JIT on stock iOS** → RandomX runs its slow interpreter. This is the
  hard ceiling until/unless we wire up SideJITServer (can run on your Proxmox
  server) to enable JIT for the app.
- **Light mode** (256 MB) is used so it fits the iPhone's app memory budget.
- **7-day cert** on a free Apple ID; SideStore auto-refreshes over WiFi.
- **Foreground only** — iOS suspends background apps; it mines while open.
