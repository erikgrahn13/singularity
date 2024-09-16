//
//  mainMacosApp.swift
//  mainMacos
//
//  Created by Erik Grahn on 2024-09-16.
//

import SwiftUI
import coreAudio

@main
struct mainMacosApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    var body: some Scene {
        WindowGroup {
            EmptyView()
                .onAppear {
                    // Call the C++ Core Audio setup when the app starts
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