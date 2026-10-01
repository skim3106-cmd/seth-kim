const root = document.querySelector("#offline-game-root");
const gameId = new URLSearchParams(window.location.search).get("game");

const makeElement = (tag, className, text) => {
  const element = document.createElement(tag);
  if (className) element.className = className;
  if (text !== undefined) element.textContent = text;
  return element;
};

const makeButton = (text, className = "offline-game-button") => {
  const button = makeElement("button", className, text);
  button.type = "button";
  return button;
};

const addHeader = (title, description) => {
  document.title = `${title} (Offline) | Seth Kim`;
  const heading = makeElement("h1", "offline-game-title", title);
  const subtitle = makeElement("p", "offline-game-description", description);
  root.append(heading, subtitle);
};

const start2048 = () => {
  addHeader("2048", "Join matching tiles to reach 2048.");
  const shell = makeElement("section", "local-game local-game-2048");
  const scoreLine = makeElement("div", "offline-game-scores");
  const scoreText = makeElement("span", "", "Score: 0");
  const bestText = makeElement("span", "", "Best: 0");
  const newGame = makeButton("New game");
  const boardElement = makeElement("div", "tile-board");
  boardElement.setAttribute("role", "grid");
  boardElement.setAttribute("aria-label", "2048 game board");
  const status = makeElement("p", "offline-game-status");
  status.setAttribute("aria-live", "polite");
  const controls = makeElement("div", "game-direction-controls");

  scoreLine.append(scoreText, bestText, newGame);
  [
    ["up", "↑"],
    ["left", "←"],
    ["down", "↓"],
    ["right", "→"]
  ].forEach(([direction, label]) => {
    const button = makeButton(label, "direction-button");
    button.dataset.move = direction;
    button.setAttribute("aria-label", `Move ${direction}`);
    controls.append(button);
  });
  shell.append(scoreLine, boardElement, controls, status);
  root.append(shell);

  let board;
  let score;
  let best = Number(localStorage.getItem("offline-2048-best") || 0);
  bestText.textContent = `Best: ${best}`;

  const addTile = () => {
    const empty = [];
    board.forEach((row, rowIndex) => row.forEach((value, columnIndex) => {
      if (value === 0) empty.push([rowIndex, columnIndex]);
    }));
    if (!empty.length) return;
    const [rowIndex, columnIndex] = empty[Math.floor(Math.random() * empty.length)];
    board[rowIndex][columnIndex] = Math.random() < 0.9 ? 2 : 4;
  };

  const render = () => {
    boardElement.replaceChildren();
    board.flat().forEach((value, index) => {
      const cell = makeElement("div", `tile-cell${value ? ` tile-${Math.min(value, 2048)}` : ""}`, value || "");
      cell.setAttribute("role", "gridcell");
      cell.setAttribute("aria-label", value ? `Tile ${value}` : "Empty");
      cell.dataset.index = index;
      boardElement.append(cell);
    });
    scoreText.textContent = `Score: ${score}`;
    bestText.textContent = `Best: ${best}`;
  };

  const reset = () => {
    board = Array.from({ length: 4 }, () => Array(4).fill(0));
    score = 0;
    status.textContent = "Use arrow keys or the direction buttons.";
    addTile();
    addTile();
    render();
  };

  const move = (direction) => {
    const previous = JSON.stringify(board);
    let gained = 0;
    for (let lineIndex = 0; lineIndex < 4; lineIndex += 1) {
      let coordinates;
      if (direction === "left") coordinates = Array.from({ length: 4 }, (_, column) => [lineIndex, column]);
      if (direction === "right") coordinates = Array.from({ length: 4 }, (_, column) => [lineIndex, 3 - column]);
      if (direction === "up") coordinates = Array.from({ length: 4 }, (_, row) => [row, lineIndex]);
      if (direction === "down") coordinates = Array.from({ length: 4 }, (_, row) => [3 - row, lineIndex]);
      const values = coordinates.map(([row, column]) => board[row][column]).filter(Boolean);
      const merged = [];
      for (let index = 0; index < values.length; index += 1) {
        if (values[index] === values[index + 1]) {
          merged.push(values[index] * 2);
          gained += values[index] * 2;
          index += 1;
        } else {
          merged.push(values[index]);
        }
      }
      coordinates.forEach(([row, column], index) => {
        board[row][column] = merged[index] || 0;
      });
    }
    if (previous === JSON.stringify(board)) return;
    score += gained;
    best = Math.max(best, score);
    localStorage.setItem("offline-2048-best", String(best));
    addTile();
    render();
    const won = board.some((row) => row.includes(2048));
    const movesRemain = board.some((row, rowIndex) => row.some((value, columnIndex) => {
      if (!value) return true;
      return rowIndex < 3 && board[rowIndex + 1][columnIndex] === value
        || columnIndex < 3 && row[columnIndex + 1] === value;
    }));
    status.textContent = won ? "2048 reached! Keep going or start a new game."
      : movesRemain ? "" : "No moves left. Start a new game to play again.";
  };

  newGame.addEventListener("click", reset);
  controls.addEventListener("click", (event) => {
    if (event.target.dataset.move) move(event.target.dataset.move);
  });
  window.addEventListener("keydown", (event) => {
    const directions = { ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right" };
    if (directions[event.key] && gameId === "2048") {
      event.preventDefault();
      move(directions[event.key]);
    }
  });
  reset();
};

const startSnake = () => {
  addHeader("Snake", "Collect food and avoid the walls and your tail.");
  const shell = makeElement("section", "local-game local-game-snake");
  const scoreText = makeElement("p", "offline-game-scores", "Score: 0");
  const canvas = makeElement("canvas", "snake-board");
  canvas.width = 400;
  canvas.height = 400;
  canvas.setAttribute("role", "img");
  canvas.setAttribute("aria-label", "Snake game board");
  const status = makeElement("p", "offline-game-status");
  status.setAttribute("aria-live", "polite");
  const controls = makeElement("div", "game-direction-controls");
  const restart = makeButton("Start game");
  [
    ["up", "↑"],
    ["left", "←"],
    ["down", "↓"],
    ["right", "→"]
  ].forEach(([direction, label]) => {
    const button = makeButton(label, "direction-button");
    button.dataset.direction = direction;
    button.setAttribute("aria-label", `Turn ${direction}`);
    controls.append(button);
  });
  shell.append(scoreText, canvas, controls, restart, status);
  root.append(shell);

  const context = canvas.getContext("2d");
  const size = 20;
  let snake;
  let food;
  let direction;
  let nextDirection;
  let score;
  let timer;
  let playing = false;

  const paint = () => {
    context.fillStyle = "#f4f0e7";
    context.fillRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = "#d54f35";
    context.fillRect(food.x * size + 2, food.y * size + 2, size - 4, size - 4);
    snake.forEach((part, index) => {
      context.fillStyle = index === 0 ? "#176b55" : "#27866c";
      context.fillRect(part.x * size + 1, part.y * size + 1, size - 2, size - 2);
    });
  };

  const makeFood = () => {
    let candidate;
    do {
      candidate = { x: Math.floor(Math.random() * 20), y: Math.floor(Math.random() * 20) };
    } while (snake.some((part) => part.x === candidate.x && part.y === candidate.y));
    return candidate;
  };

  const finish = () => {
    clearInterval(timer);
    playing = false;
    restart.textContent = "Play again";
    status.textContent = "Game over. Start a new round when you're ready.";
  };

  const step = () => {
    direction = nextDirection;
    const head = { x: snake[0].x + direction.x, y: snake[0].y + direction.y };
    const eating = head.x === food.x && head.y === food.y;
    if (head.x < 0 || head.y < 0 || head.x >= 20 || head.y >= 20
      || snake.slice(0, eating ? undefined : -1).some((part) => part.x === head.x && part.y === head.y)) {
      finish();
      return;
    }
    snake.unshift(head);
    if (eating) {
      score += 1;
      scoreText.textContent = `Score: ${score}`;
      food = makeFood();
    } else {
      snake.pop();
    }
    paint();
  };

  const start = () => {
    clearInterval(timer);
    snake = [{ x: 10, y: 10 }, { x: 9, y: 10 }, { x: 8, y: 10 }];
    food = makeFood();
    direction = { x: 1, y: 0 };
    nextDirection = direction;
    score = 0;
    playing = true;
    scoreText.textContent = "Score: 0";
    status.textContent = "Use arrow keys or the direction buttons.";
    restart.textContent = "Restart";
    paint();
    timer = setInterval(step, 140);
  };

  const turn = (name) => {
    if (!playing) return;
    const options = {
      up: { x: 0, y: -1 },
      down: { x: 0, y: 1 },
      left: { x: -1, y: 0 },
      right: { x: 1, y: 0 }
    };
    const requested = options[name];
    if (requested.x + direction.x !== 0 || requested.y + direction.y !== 0) nextDirection = requested;
  };

  restart.addEventListener("click", start);
  controls.addEventListener("click", (event) => {
    if (event.target.dataset.direction) turn(event.target.dataset.direction);
  });
  window.addEventListener("keydown", (event) => {
    const directions = { ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right" };
    if (directions[event.key] && gameId === "snake") {
      event.preventDefault();
      turn(directions[event.key]);
    }
  });
  paint();
};

const startTicTacToe = () => {
  addHeader("Tic-Tac-Toe", "A two-player game on one screen.");
  const shell = makeElement("section", "local-game local-game-tic-tac-toe");
  const status = makeElement("p", "offline-game-status", "X's turn");
  status.setAttribute("aria-live", "polite");
  const board = makeElement("div", "tic-tac-toe-board");
  board.setAttribute("role", "group");
  board.setAttribute("aria-label", "Tic-Tac-Toe board");
  const reset = makeButton("New game");
  const cells = [];
  let squares = Array(9).fill("");
  let current = "X";
  let finished = false;
  const wins = [[0, 1, 2], [3, 4, 5], [6, 7, 8], [0, 3, 6], [1, 4, 7], [2, 5, 8], [0, 4, 8], [2, 4, 6]];

  for (let index = 0; index < 9; index += 1) {
    const cell = makeButton("", "tic-tac-toe-cell");
    cell.dataset.index = index;
    cell.setAttribute("aria-label", `Square ${index + 1}, empty`);
    cells.push(cell);
    board.append(cell);
  }
  shell.append(status, board, reset);
  root.append(shell);

  const render = () => {
    cells.forEach((cell, index) => {
      cell.textContent = squares[index];
      cell.disabled = finished || Boolean(squares[index]);
      cell.setAttribute("aria-label", `Square ${index + 1}, ${squares[index] || "empty"}`);
    });
  };

  board.addEventListener("click", (event) => {
    const index = Number(event.target.dataset.index);
    if (!Number.isInteger(index) || squares[index] || finished) return;
    squares[index] = current;
    const winner = wins.some((line) => line.every((position) => squares[position] === current));
    if (winner) {
      status.textContent = `${current} wins!`;
      finished = true;
    } else if (squares.every(Boolean)) {
      status.textContent = "It's a draw.";
      finished = true;
    } else {
      current = current === "X" ? "O" : "X";
      status.textContent = `${current}'s turn`;
    }
    render();
  });

  reset.addEventListener("click", () => {
    squares = Array(9).fill("");
    current = "X";
    finished = false;
    status.textContent = "X's turn";
    render();
  });
  render();
};

if (gameId === "2048") {
  start2048();
} else if (gameId === "snake") {
  startSnake();
} else if (gameId === "tic-tac-toe") {
  startTicTacToe();
} else {
  addHeader("Offline game", "Choose a bundled game from the Pizza Games list.");
}