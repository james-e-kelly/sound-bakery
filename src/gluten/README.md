# Gluten

Gluten is a UI wrapper of ImGui that tries to simplify application development in C++. The main concepts it adds are:
- Application - The main class to create the UI/UX
- Managers - Classes that control behaviour. They can create and destroy widgets, respond to input and so on. For example, an "Intro Manager" could handle logins
- Subsystems - Classes that extend the application, adding rendering, video, or audio support
- Widgets - Similar to widgets in Unreal Engine, widgets hold position, stretch, and size information
- Elements - Small pieces of reusable UI. These are basically extensions of ImGui widgets like buttons and sliders. A widget could use a "button" element to place a button in the top right corner, with a set padding and 4px away from the right