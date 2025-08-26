package util

import (
	"fmt"
	"sync"
)

type Message struct {
	Topic string
	Data  interface{}
}

type Subscriber struct {
	id      string
	message chan *Message
}

func (s *Subscriber) GetMessage() <-chan *Message {
	return s.message
}

type PubSub struct {
	subscribers map[string]map[string]*Subscriber
	mu          sync.RWMutex
}

func NewPubSub() *PubSub {
	return &PubSub{
		subscribers: make(map[string]map[string]*Subscriber),
	}
}
func (ps *PubSub) Subscribe(topic string, id string) (*Subscriber, error) {
	ps.mu.Lock()
	defer ps.mu.Unlock()

	if _, ok := ps.subscribers[topic]; !ok {
		ps.subscribers[topic] = make(map[string]*Subscriber)
	}

	if _, ok := ps.subscribers[topic][id]; ok {
		return nil, fmt.Errorf("subscriber with id %s already exists for topic %s", id, topic)
	}

	s := &Subscriber{
		id:      id,
		message: make(chan *Message, 10),
	}
	ps.subscribers[topic][id] = s
	return s, nil
}

func (ps *PubSub) Unsubscribe(topic string, id string) {
	ps.mu.Lock()
	defer ps.mu.Unlock()

	if _, ok := ps.subscribers[topic]; !ok {
		return
	}

	if s, ok := ps.subscribers[topic][id]; ok {
		close(s.message)
		delete(ps.subscribers[topic], id)
	}

	if len(ps.subscribers[topic]) == 0 {
		delete(ps.subscribers, topic)
	}
}

func (ps *PubSub) Publish(topic string, data interface{}) {
	ps.mu.RLock()
	defer ps.mu.RUnlock()

	message := &Message{
		Topic: topic,
		Data:  data,
	}

	if subs, ok := ps.subscribers[topic]; ok {
		for _, s := range subs {
			select {
			case s.message <- message:
			default:
				// TODO: fail to send message
			}
		}
	}
}
