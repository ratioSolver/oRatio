export function blink(element: HTMLElement, duration: number = 1000): void {
  element.classList.add('blink');
  setTimeout(() => element.classList.remove('blink'), duration);
}