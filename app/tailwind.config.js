/** @type {import('tailwindcss').Config} */

const defaultTheme = require('tailwindcss/defaultTheme');
module.exports = {
  content: ['./src/**/*.{html,js}'],
  theme: {
    extend: {
      fontFamily: {
        sans: ['Inter', ...defaultTheme.fontFamily.sans],
        serif: [...defaultTheme.fontFamily.serif],
        code: 'Fira Code',
      },
      colors: {
        brand: {
          primary: '#9F4160',
          'primary-accent': '#BF8095',
          secondary: '#F4E9EA',
          tertiary: '#FAF8F8',
          white: '#ffffff',
          black: '#000000',
          gray: '#E3E3E3',
          'dark-gray': '#C3C3C3',
          neutral: '#111111',
          border: '#E3E3E3',
        },
      },
    },
  },
  plugins: [],
};
