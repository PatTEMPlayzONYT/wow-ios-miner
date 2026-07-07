import SwiftUI

@main
struct WowLottoApp: App {
    var body: some Scene {
        WindowGroup { ContentView() }
    }
}

struct ContentView: View {
    @State private var status = "Tap to measure this iPhone's RandomX speed"
    @State private var hashrate: Double? = nil
    @State private var running = false

    @State private var threads: Int32 = 0
    @State private var mode: Int32 = -1
    private let seconds: Int32 = 8   // timed run across all cores

    private func modeName(_ m: Int32) -> String {
        switch m {
        case 1:  return "JIT + fast mode 🚀"
        case 2:  return "JIT + light mode"
        default: return "interpreter + light (no JIT)"
        }
    }

    var body: some View {
        VStack(spacing: 22) {
            Text("WOW Lotto").font(.largeTitle.bold())
            Text("Step 1 — hashrate test").foregroundStyle(.secondary)

            if running {
                ProgressView().padding(.top, 8)
            }
            if let h = hashrate {
                Text(String(format: "%.1f H/s", h))
                    .font(.system(size: 52, weight: .heavy))
                    .foregroundStyle(.orange)
                if threads > 0 {
                    Text("\(threads) cores · \(modeName(mode))")
                        .font(.subheadline).foregroundStyle(.secondary)
                }
            }

            Text(status)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal)

            Button(running ? "Hashing…" : "Run benchmark") {
                startBenchmark()
            }
            .buttonStyle(.borderedProminent)
            .disabled(running)

            Text("Interpreter + light mode (the only config stock iOS allows). "
                 + "This is the floor; JIT/fast-mode would be faster.")
                .font(.caption2)
                .foregroundStyle(.tertiary)
                .multilineTextAlignment(.center)
                .padding(.horizontal)
                .padding(.top, 8)
        }
        .padding()
    }

    private func startBenchmark() {
        running = true
        hashrate = nil
        status = "Hashing for \(seconds)s on all cores… keep the app open."
        DispatchQueue.global(qos: .userInitiated).async {
            var t: Int32 = 0
            var m: Int32 = -1
            let hs = randomx_benchmark(seconds, &t, &m)
            DispatchQueue.main.async {
                running = false
                if hs < 0 {
                    status = "Out of memory allocating the RandomX cache."
                } else {
                    hashrate = hs
                    threads = t
                    mode = m
                    status = m == 0
                        ? "No JIT — start SideJITServer, relaunch, try again."
                        : "Done — JIT active. 🎉"
                }
            }
        }
    }
}
