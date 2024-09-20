//
//  mainMacosApp.swift
//  mainMacos
//
//  Created by Erik Grahn on 2024-09-16.
//

import SwiftUI
import coreAudio
import skiaGui

struct SkiaView: NSViewRepresentable {
    func makeNSView(context: Context) -> SkiaNSView {
        let view = SkiaNSView()
        return view
    }

    func updateNSView(_ nsView: SkiaNSView, context: Context) {
        nsView.setNeedsDisplay(nsView.bounds)  // Request to redraw the view
    }

}

class SkiaNSView: NSView {

override func draw(_ dirtyRect: NSRect) {
    // Get the current graphics context (CGContext)
    guard let context = NSGraphicsContext.current?.cgContext else {
        print("Error: No CGContext found!")
        return
    }

    drawSkia(context)
}

override func viewDidMoveToWindow() {
    self.wantsLayer = true
    self.layer?.displayIfNeeded()  // Force immediate redraw
}

}


@main
struct mainMacosApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    var body: some Scene {
        WindowGroup {
            SkiaView()
                .onAppear {
                    // Call the C++ Core Audio setup when the app starts
                    setupSkia();
                    setupCoreAudio()
                }
        }
    }
}

class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationWillTerminate(_ notification: Notification) {
        // Call the C++ Core Audio cleanup functions when the app is closing
        stopCoreAudio()
    }
}