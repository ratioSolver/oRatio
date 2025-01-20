import { ComputePositionConfig } from '@floating-ui/dom';

declare module 'cytoscape-popper' {

  interface PopperOptions extends ComputePositionConfig {
  }

  interface PopperInstance {
    update(): void;
  }
}

import cytoscape from 'cytoscape';
import dagre from 'cytoscape-dagre';
import cytoscapePopper, { PopperInstance, PopperOptions, RefElement } from 'cytoscape-popper';
import {
  computePosition,
  flip,
  shift,
  limitShift,
} from '@floating-ui/dom';

export { solver } from './solver/solver';
export { SolverGraph } from './solver/solver_graph';
export { TimelinesChart } from './solver/solver_timelines';
export * from './solver/solver_components';

function popperFactory(ref: RefElement, content: HTMLElement, options?: PopperOptions): PopperInstance {
  const popperOptions = { middleware: [flip(), shift({ limiter: limitShift() })], ...options, };

  function update() {
    computePosition(ref, content, popperOptions).then(({ x, y }) => {
      Object.assign(content.style, {
        left: `${x}px`,
        top: `${y}px`,
      });
    });
  }
  update();
  return { update };
}

cytoscape.use(dagre);
cytoscape.use(cytoscapePopper(popperFactory));