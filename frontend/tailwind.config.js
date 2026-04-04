/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        primary: '#1890ff',
        success: '#52c41a',
        warning: '#faad14',
        danger: '#ff4d4f',
        dark: {
          bg: '#141414',
          card: '#1f1f1f',
          border: '#434343',
        },
      },
    },
  },
  plugins: [],
}
