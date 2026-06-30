#!/bin/bash

# GNUnet Messenger Client Test Script
# Assumes server is already installed and running
# Usage: ./run_client_tests.sh

set -e  # Exit on error

echo "========================================="
echo "GNUnet Messenger Client Tests"
echo "========================================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Check ADB
echo "Checking prerequisites..."
if ! command -v adb &> /dev/null; then
    print_error "adb not found"
    exit 1
fi
print_success "ADB found"

# Check device
echo ""
echo "Checking Android device connection..."
DEVICES=$(adb devices | grep -v "List of devices" | grep -v "^$" | wc -l)
if [ "$DEVICES" -eq 0 ]; then
    print_error "No Android device connected"
    exit 1
fi
if [ "$DEVICES" -gt 1 ]; then
    print_warning "Multiple devices detected. Using first device."
fi
print_success "Device connected"

# Check server is installed
echo ""
echo "Checking for GNUnet server app..."
if ! adb shell pm list packages | grep -q "org.gnu.gnunet"; then
    print_error "Server app not installed"
    echo "Please install server first from gnunet-android directory:"
    echo "  cd ../gnunet-android/android_studio && ./gradlew installDebug"
    exit 1
fi
print_success "Server app found: org.gnu.gnunet"

# Check if server is running
echo ""
echo "Checking server status..."
SERVER_PID=$(adb shell pidof org.gnu.gnunet || echo "")
if [ -z "$SERVER_PID" ]; then
    print_warning "Server not running - starting it..."
    adb shell am start -n org.gnu.gnunet/.MainActivity > /dev/null 2>&1
    sleep 3
    print_success "Server started"
else
    print_success "Server already running (PID: $SERVER_PID)"
fi

# Build client APK
echo ""
echo "========================================="
echo "Step 1: Build Client App"
echo "========================================="
echo ""

if ./GNUnetMessenger/gradlew assembleDebug > /dev/null 2>&1; then
    print_success "Client APK built successfully"
else
    print_error "Failed to build client APK"
    exit 1
fi

# Install client
echo ""
echo "========================================="
echo "Step 2: Install/Update Client App"
echo "========================================="
echo ""

if ./GNUnetMessenger/gradlew installDebug > /dev/null 2>&1; then
    print_success "Client app installed/updated"
else
    print_error "Failed to install client app"
    exit 1
fi

# Run tests
echo ""
echo "========================================="
echo "Step 3: Run Instrumentation Tests"
echo "========================================="
echo ""

./GNUnetMessenger/gradlew connectedAndroidTest
TEST_RESULT=$?

# Display results
echo ""
echo "========================================="
echo "Test Results"
echo "========================================="

if [ $TEST_RESULT -eq 0 ]; then
    print_success "All tests passed!"
    echo ""
    echo "Test report location:"
    echo "  ./GNUnetMessenger/app/build/reports/androidTests/connected/index.html"
else
    print_error "Some tests failed"
    echo ""
    echo "Test report location:"
    echo "  ./GNUnetMessenger/app/build/reports/androidTests/connected/index.html"
    echo ""
    echo "Check logs for details:"
    echo "  adb logcat | grep -E 'GnunetChatIpcService|GnunetChatBoundService'"
fi

# Offer to view logs
echo ""
read -p "View logs now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "========================================="
    echo "GNUnet Logs (Ctrl+C to exit)"
    echo "========================================="
    echo ""
    adb logcat | grep -E "GnunetChatIpcService|GnunetChatBoundService|GNUNET"
fi

echo ""
echo "========================================="
echo "Test execution completed!"
echo "========================================="
