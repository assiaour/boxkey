const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;

// Security middleware
app.use(cors({
  origin: '*', // Allow all origins for development
  methods: ['GET', 'POST'],
  allowedHeaders: ['Content-Type']
}));
app.use(express.json());

// Connect to MongoDB Atlas
const mongoUri = process.env.MONGODB_URI;
if (!mongoUri) {
  console.error('MONGODB_URI environment variable is not set');
  process.exit(1);
}

mongoose.connect(mongoUri, {
  useNewUrlParser: true,
  useUnifiedTopology: true,
})
.then(() => console.log('Connected to MongoDB'))
.catch(err => {
  console.error('MongoDB connection error:', err);
  process.exit(1);
});

// Password Schema
const passwordSchema = new mongoose.Schema({
  password: { type: String, required: true },
  clientId: { type: String, required: true },
  expiryDate: { type: Date, required: true },
  isActive: { type: Boolean, default: true },
  createdAt: { type: Date, default: Date.now }
});

const Password = mongoose.model('Password', passwordSchema);

// API Endpoints
app.post('/api/passwords', async (req, res) => {
  try {
    const { password, clientId, expiryHours } = req.body;
    
    // Validate input
    if (!password || !clientId || !expiryHours) {
      return res.status(400).json({ error: 'Missing required fields' });
    }

    const expiryDate = new Date();
    expiryDate.setHours(expiryDate.getHours() + parseInt(expiryHours));

    const newPassword = new Password({
      password,
      clientId,
      expiryDate,
      isActive: true
    });

    await newPassword.save();
    res.status(201).json(newPassword);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Endpoint for ESP8266 to verify password
app.post('/api/verify', async (req, res) => {
  try {
    const { password } = req.body;
    
    if (!password) {
      return res.status(400).json({ error: 'Password is required' });
    }

    const validPassword = await Password.findOne({
      password,
      isActive: true,
      expiryDate: { $gt: new Date() }
    });

    if (validPassword) {
      // Deactivate the password after successful verification
      validPassword.isActive = false;
      await validPassword.save();
      
      res.json({ valid: true, message: 'Password is valid' });
    } else {
      res.json({ valid: false, message: 'Invalid or expired password' });
    }
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ status: 'ok' });
});

// Test endpoint to check database connection and list passwords
app.get('/api/test', async (req, res) => {
  try {
    const passwords = await Password.find({});
    res.json({
      status: 'success',
      connection: 'MongoDB connected',
      passwordCount: passwords.length,
      passwords: passwords
    });
  } catch (error) {
    res.status(500).json({
      status: 'error',
      message: error.message
    });
  }
});

// Start server
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
}); 