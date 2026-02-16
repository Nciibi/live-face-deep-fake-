#!/bin/bash

# Live Face Deep Fake - GitHub Push Instructions
# Run this script from the project root directory to push to GitHub

echo "🚀 Live Face Deep Fake - GitHub Setup Guide"
echo "============================================"
echo ""

# Check if git is initialized
if [ ! -d .git ]; then
    echo "1️⃣  Initializing git repository..."
    git init
    echo ""
fi

# Add all files
echo "2️⃣  Adding all files to git..."
git add .
echo "✓ Files staged"
echo ""

# First commit
echo "3️⃣  Creating initial commit..."
if ! git rev-parse HEAD > /dev/null 2>&1; then
    git config user.name "Your Name" || true
    git config user.email "your.email@example.com" || true
    git commit -m "Initial commit: Real-time face swapping with INSwapper integration" || true
fi
echo "✓ Commit created"
echo ""

# Rename branch to main
echo "4️⃣  Setting branch to 'main'..."
git branch -M main
echo "✓ Branch set to main"
echo ""

# Add remote
echo "5️⃣  Adding GitHub remote..."
echo ""
echo "   ⚠️  You need to:"
echo "   1. Create a new repository on GitHub (https://github.com/new)"
echo "   2. Name it: live-face-deep-fake"
echo "   3. DO NOT initialize with README"
echo ""
read -p "Enter your GitHub username: " USERNAME

REPO_URL="https://github.com/${USERNAME}/live-face-deep-fake.git"
echo ""
echo "   Adding remote: $REPO_URL"

git remote remove origin 2>/dev/null || true
git remote add origin "$REPO_URL"
echo "✓ Remote added"
echo ""

# Push to GitHub
echo "6️⃣  Pushing to GitHub..."
echo ""
git push -u origin main

echo ""
echo "✅ Done! Your project is now on GitHub"
echo ""
echo "Next steps:"
echo "  1. Add GitHub topics: face-swapping, deepfake, opencv, onnx, inswapper"
echo "  2. Update repository description"
echo "  3. Enable Discussions for community"
echo "  4. Share the link: https://github.com/${USERNAME}/live-face-deep-fake"

