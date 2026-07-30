import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "AQ-CONTROL",
  description: "Dashboard remoto do AQ-CONTROL",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="pt">
      <body className="bg-slate-900 text-white min-h-screen">{children}</body>
    </html>
  );
}
