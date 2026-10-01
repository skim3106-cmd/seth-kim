const searchInput = document.querySelector("#game-search");
const gameList = document.querySelector("#game-list");
const gameCount = document.querySelector("#game-count");
const emptyMessage = document.querySelector("#game-empty");

let games = [];

const offlineGames = [
  { name: "2048 (Offline)", id: "2048", offline: true },
  { name: "Snake (Offline)", id: "snake", offline: true },
  { name: "Tic-Tac-Toe (Offline)", id: "tic-tac-toe", offline: true }
];

const renderGames = () => {
  const query = searchInput.value.trim().toLocaleLowerCase();
  const filteredGames = games.filter((game) => game.name.toLocaleLowerCase().includes(query));
  const fragment = document.createDocumentFragment();

  filteredGames.forEach((game) => {
    const item = document.createElement("li");
    const link = document.createElement("a");
    const name = document.createElement("span");
    const arrow = document.createElement("span");

    link.href = game.offline
      ? `offline-games.html?game=${encodeURIComponent(game.id)}`
      : `pizza-play.html?game=${encodeURIComponent(game.url)}`;
    name.textContent = game.name;
    arrow.textContent = "→";
    arrow.setAttribute("aria-hidden", "true");
    link.append(name, arrow);
    item.append(link);
    fragment.append(item);
  });

  gameList.replaceChildren(fragment);
  gameCount.textContent = `${filteredGames.length} of ${games.length} games`;
  emptyMessage.hidden = filteredGames.length > 0;
};

searchInput.addEventListener("input", renderGames);
games = offlineGames;
renderGames();

fetch("pizza-games.json")
  .then((response) => {
    if (!response.ok) throw new Error("Could not load game list");
    return response.json();
  })
  .then((catalog) => {
    games = [...offlineGames, ...catalog]
      .sort((first, second) => first.name.localeCompare(second.name));
    renderGames();
  })
  .catch(() => {
    gameCount.textContent = "Showing offline games; the online catalog is unavailable.";
  });