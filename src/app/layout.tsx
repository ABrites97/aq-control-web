import "./globals.css";
import type { ReactNode } from "react";

export const metadata = {
  title: "AQ-CONTROL",
  description: "Dashboard remoto do AQ-CONTROL",
};

export const viewport = {
  width: "device-width",
  initialScale: 1,
};

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="pt">
      <body>{children}</body>
    </html>
  );
}
