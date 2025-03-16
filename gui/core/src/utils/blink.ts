export function blink(element: HTMLElement): void {
  element.classList.add('blink');
  setTimeout(() => element.classList.remove('blink'), 1000);
}