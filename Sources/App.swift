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

    // Number of hashes to time. Interpreter+light mode is slow, so keep it
    // modest — this can still take 20-90s on an iPhone 13.
    private let iterations: Int32 = 128

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
        status = "Hashing \(iterations) rounds… keep the app open."
        DispatchQueue.global(qos: .userInitiated).async {
            let hs = randomx_benchmark(iterations)
            DispatchQueue.main.async {
                running = false
                if hs < 0 {
                    status = hs == -1
                        ? "Out of memory allocating the RandomX cache."
                        : "Failed to create the RandomX VM (code \(Int(hs)))."
                } else {
                    hashrate = hs
                    status = "Done. This is your iPhone 13's real RandomX rate."
                }
            }
        }
    }
}
