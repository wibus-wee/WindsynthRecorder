import SwiftUI

class NotificationManager: ObservableObject {
    static let shared = NotificationManager()
    
    @Published var message: String = ""
    @Published var isError: Bool = false
    @Published var isShowing: Bool = false
    
    private init() {}
    
    func showError(message: String) {
        self.message = message
        self.isError = true
        self.isShowing = true
        
        hideAfterDelay()
    }
    
    func showSuccess(message: String) {
        self.message = message
        self.isError = false
        self.isShowing = true
        
        hideAfterDelay()
    }
    
    private func hideAfterDelay() {
        DispatchQueue.main.asyncAfter(deadline: .now() + 3) { [weak self] in
            withAnimation {
                self?.isShowing = false
            }
        }
    }
} 