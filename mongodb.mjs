import { MongoClient } from 'mongodb';

let client;
let connectionPromise;

export async function connectToMongoDB() {
  if (client) {
    return client;
  }

  if (connectionPromise) {
    return connectionPromise;
  }

  const uri = process.env.MONGODB_URI;
  if (!uri) {
    throw new Error('Set the MONGODB_URI environment variable before connecting.');
  }

  const nextClient = new MongoClient(uri);
  connectionPromise = nextClient.connect()
    .then(() => {
      client = nextClient;
      return client;
    })
    .catch(async (error) => {
      await nextClient.close();
      throw error;
    })
    .finally(() => {
      connectionPromise = undefined;
    });

  return connectionPromise;
}

export async function disconnectFromMongoDB() {
  if (connectionPromise) {
    await connectionPromise.catch(() => {});
  }

  if (!client) {
    return;
  }

  const connectedClient = client;
  client = undefined;
  await connectedClient.close();
}