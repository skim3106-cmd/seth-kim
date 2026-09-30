const params = new URLSearchParams(window.location.search);
const requestedPath = params.get("game");
const gameTitle = document.querySelector("#game-title");
const gameFrame = document.querySelector("#game-frame");
const openGame = document.querySelector("#open-game");
const gameError = document.querySelector("#game-error");

fetch("pizza-games.json")
  .then((response) => {
    if (!response.ok) throw new Error("Could not load game list");
    return response.json();
  })
  .then((games) => {
    const game = games.find((entry) => entry.url === requestedPath);

    if (!game || !game.url.startsWith("/g/")) throw new Error("Unknown game");

    const gameUrl = new URL(game.url, "https://pizzaedition.com").href;
    document.title = `${game.name} | Pizza Edition`;
    gameTitle.textContent = game.name;
    gameFrame.title = game.name;
    gameFrame.src = gameUrl;
    openGame.href = gameUrl;
  })
  .catch(() => {
    gameFrame.hidden = true;
    openGame.hidden = true;
    gameError.hidden = false;
    gameTitle.textContent = "Game not found";
  });